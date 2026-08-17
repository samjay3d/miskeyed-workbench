#include <slang_qrhi/LspClient.h>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCoreApplication>

namespace slang_qrhi {

LspClient::LspClient(QObject* parent) : QObject(parent) {}

LspClient::~LspClient()
{
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        // Politely ask the server to exit; fall back to a hard kill.
        writeMessage({{"jsonrpc", QStringLiteral("2.0")}, {"id", m_nextId++},
                      {"method", QStringLiteral("shutdown")}, {"params", QJsonValue::Null}});
        writeMessage({{"jsonrpc", QStringLiteral("2.0")},
                      {"method", QStringLiteral("exit")}, {"params", QJsonValue::Null}});
        m_proc->closeWriteChannel();
        if (!m_proc->waitForFinished(500)) {
            m_proc->kill();
            m_proc->waitForFinished(500);
        }
    }
}

void LspClient::start(const QString& serverPath)
{
    m_proc = new QProcess(this);
    connect(m_proc, &QProcess::readyReadStandardOutput, this, &LspClient::onReadyRead);
    m_proc->start(serverPath, {});

    QJsonObject completion{
        {QStringLiteral("completionItem"), QJsonObject{{QStringLiteral("snippetSupport"), false},
            {QStringLiteral("documentationFormat"), QJsonArray{QStringLiteral("plaintext"), QStringLiteral("markdown")}}}},
        {QStringLiteral("contextSupport"), true}};
    QJsonObject textDocument{
        {QStringLiteral("completion"), completion},
        {QStringLiteral("hover"), QJsonObject{{QStringLiteral("contentFormat"),
            QJsonArray{QStringLiteral("plaintext"), QStringLiteral("markdown")}}}},
        {QStringLiteral("signatureHelp"), QJsonObject{}},
        {QStringLiteral("definition"), QJsonObject{}},
        {QStringLiteral("publishDiagnostics"), QJsonObject{}}};
    QJsonObject params{
        {QStringLiteral("processId"), QJsonValue::Null},
        {QStringLiteral("rootUri"), QJsonValue::Null},
        {QStringLiteral("clientInfo"), QJsonObject{{QStringLiteral("name"), QStringLiteral("slang-qt")}}},
        {QStringLiteral("capabilities"), QJsonObject{{QStringLiteral("textDocument"), textDocument}}}};

    sendRequestNow(QStringLiteral("initialize"), params, [this](const QJsonObject&) {
        writeMessage({{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                      {QStringLiteral("method"), QStringLiteral("initialized")},
                      {QStringLiteral("params"), QJsonObject{}}});
        m_ready = true;
        const auto queued = m_queued;
        m_queued.clear();
        for (const auto& fn : queued) fn();
        emit ready();
    });
}

void LspClient::openDocument(const QString& uri, const QString& text)
{
    m_versions[uri] = 1;
    postNotification(QStringLiteral("textDocument/didOpen"), {
        {QStringLiteral("textDocument"), QJsonObject{
            {QStringLiteral("uri"), uri},
            {QStringLiteral("languageId"), QStringLiteral("slang")},
            {QStringLiteral("version"), 1},
            {QStringLiteral("text"), text}}}});
}

void LspClient::updateDocument(const QString& uri, const QString& text)
{
    const int version = (m_versions.value(uri, 1)) + 1;
    m_versions[uri] = version;
    // Full-document sync: a change event with no `range` replaces the whole document,
    // which every server accepts regardless of its advertised sync kind.
    postNotification(QStringLiteral("textDocument/didChange"), {
        {QStringLiteral("textDocument"), QJsonObject{
            {QStringLiteral("uri"), uri}, {QStringLiteral("version"), version}}},
        {QStringLiteral("contentChanges"), QJsonArray{QJsonObject{{QStringLiteral("text"), text}}}}});
}

void LspClient::closeDocument(const QString& uri)
{
    postNotification(QStringLiteral("textDocument/didClose"), {
        {QStringLiteral("textDocument"), QJsonObject{{QStringLiteral("uri"), uri}}}});
}

QJsonObject LspClient::positionParams(const QString& uri, int line, int character) const
{
    return QJsonObject{
        {QStringLiteral("textDocument"), QJsonObject{{QStringLiteral("uri"), uri}}},
        {QStringLiteral("position"), QJsonObject{
            {QStringLiteral("line"), line}, {QStringLiteral("character"), character}}}};
}

void LspClient::requestCompletion(const QString& uri, int line, int character, JsonCallback cb)
{ postRequest(QStringLiteral("textDocument/completion"), positionParams(uri, line, character), std::move(cb)); }

void LspClient::requestHover(const QString& uri, int line, int character, JsonCallback cb)
{ postRequest(QStringLiteral("textDocument/hover"), positionParams(uri, line, character), std::move(cb)); }

void LspClient::requestSignatureHelp(const QString& uri, int line, int character, JsonCallback cb)
{ postRequest(QStringLiteral("textDocument/signatureHelp"), positionParams(uri, line, character), std::move(cb)); }

void LspClient::requestDefinition(const QString& uri, int line, int character, JsonCallback cb)
{ postRequest(QStringLiteral("textDocument/definition"), positionParams(uri, line, character), std::move(cb)); }

void LspClient::postNotification(const QString& method, const QJsonObject& params)
{
    if (!m_ready) { m_queued.append([this, method, params] { postNotification(method, params); }); return; }
    writeMessage({{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                  {QStringLiteral("method"), method}, {QStringLiteral("params"), params}});
}

void LspClient::postRequest(const QString& method, const QJsonObject& params, JsonCallback cb)
{
    if (!m_ready) { m_queued.append([this, method, params, cb] { postRequest(method, params, cb); }); return; }
    sendRequestNow(method, params, std::move(cb));
}

void LspClient::sendRequestNow(const QString& method, const QJsonObject& params, JsonCallback cb)
{
    const int id = m_nextId++;
    if (cb) m_pending.insert(id, std::move(cb));
    writeMessage({{QStringLiteral("jsonrpc"), QStringLiteral("2.0")}, {QStringLiteral("id"), id},
                  {QStringLiteral("method"), method}, {QStringLiteral("params"), params}});
}

void LspClient::writeMessage(const QJsonObject& message)
{
    if (!m_proc) return;
    const QByteArray body = QJsonDocument(message).toJson(QJsonDocument::Compact);
    m_proc->write("Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n");
    m_proc->write(body);
}

void LspClient::onReadyRead()
{
    m_buffer += m_proc->readAllStandardOutput();
    for (;;) {
        const int headerEnd = m_buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0) return;
        int contentLength = -1;
        for (const QByteArray& line : m_buffer.left(headerEnd).split('\n')) {
            const QByteArray trimmed = line.trimmed();
            if (trimmed.toLower().startsWith("content-length:"))
                contentLength = trimmed.mid(trimmed.indexOf(':') + 1).trimmed().toInt();
        }
        if (contentLength < 0) { m_buffer.remove(0, headerEnd + 4); continue; }
        const int bodyStart = headerEnd + 4;
        if (m_buffer.size() < bodyStart + contentLength) return;   // wait for the rest
        const QByteArray body = m_buffer.mid(bodyStart, contentLength);
        m_buffer.remove(0, bodyStart + contentLength);
        const QJsonDocument doc = QJsonDocument::fromJson(body);
        if (doc.isObject()) handleMessage(doc.object());
    }
}

void LspClient::handleMessage(const QJsonObject& message)
{
    const bool hasId = message.contains(QStringLiteral("id"));
    const bool hasMethod = message.contains(QStringLiteral("method"));

    if (hasId && hasMethod) {
        // A request from the server -> reply so it does not block waiting on us.
        const QString method = message.value(QStringLiteral("method")).toString();
        QJsonObject response{{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                             {QStringLiteral("id"), message.value(QStringLiteral("id"))}};
        if (method == QStringLiteral("workspace/configuration")) {
            const QJsonArray items = message.value(QStringLiteral("params")).toObject()
                                         .value(QStringLiteral("items")).toArray();
            QJsonArray result;
            for (int i = 0; i < items.size(); ++i) result.append(QJsonValue::Null);
            response.insert(QStringLiteral("result"), result);
        } else {
            response.insert(QStringLiteral("result"), QJsonValue::Null);
        }
        writeMessage(response);
        return;
    }

    if (hasMethod) {
        const QString method = message.value(QStringLiteral("method")).toString();
        if (method == QStringLiteral("textDocument/publishDiagnostics")) {
            const QJsonObject params = message.value(QStringLiteral("params")).toObject();
            const QString uri = params.value(QStringLiteral("uri")).toString();
            QList<LspDiagnostic> diagnostics;
            for (const QJsonValue& v : params.value(QStringLiteral("diagnostics")).toArray()) {
                const QJsonObject d = v.toObject();
                const QJsonObject range = d.value(QStringLiteral("range")).toObject();
                const QJsonObject s = range.value(QStringLiteral("start")).toObject();
                const QJsonObject e = range.value(QStringLiteral("end")).toObject();
                LspDiagnostic diag;
                diag.range = {s.value(QStringLiteral("line")).toInt(), s.value(QStringLiteral("character")).toInt(),
                              e.value(QStringLiteral("line")).toInt(), e.value(QStringLiteral("character")).toInt()};
                diag.severity = d.value(QStringLiteral("severity")).toInt(1);
                diag.message = d.value(QStringLiteral("message")).toString();
                diagnostics.append(diag);
            }
            emit diagnosticsReceived(uri, diagnostics);
        }
        return;
    }

    if (hasId) {
        const int id = message.value(QStringLiteral("id")).toInt();
        const auto it = m_pending.find(id);
        if (it != m_pending.end()) {
            const JsonCallback cb = it.value();
            m_pending.erase(it);
            if (cb) cb(message);
        }
    }
}

} // namespace slang_qrhi

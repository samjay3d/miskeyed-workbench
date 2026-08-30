#pragma once

#include <miskeyed/workbench/Export.h>
#include <QObject>
#include <QString>
#include <QList>
#include <QHash>
#include <QJsonObject>
#include <functional>

class QProcess;

namespace miskeyed::workbench::slang_rhi {

// A source range in LSP (0-based line, UTF-16 code-unit character offsets).
struct LspRange {
    int startLine = 0;
    int startChar = 0;
    int endLine = 0;
    int endChar = 0;
};

// One diagnostic published by the language server. Severity: 1=Error 2=Warning 3=Info 4=Hint.
struct LspDiagnostic {
    LspRange range;
    int severity = 1;
    QString message;
};

// A thin Language Server Protocol client that drives Slang's `slangd.exe` over stdio.
// It speaks JSON-RPC framed with Content-Length headers, buffers requests until the
// server has completed the `initialize` handshake, and surfaces the language features
// (completion, hover, signature help, definition, live diagnostics) the editor needs.
class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT LspClient final : public QObject {
    Q_OBJECT
public:
    explicit LspClient(QObject* parent = nullptr);
    ~LspClient() override;

    // Launches the given `slangd` executable and starts the initialize handshake.
    void start(const QString& serverPath);
    bool isReady() const { return m_ready; }

    void openDocument(const QString& uri, const QString& text);
    void updateDocument(const QString& uri, const QString& text);
    void closeDocument(const QString& uri);

    using JsonCallback = std::function<void(const QJsonObject& message)>;
    void requestCompletion(const QString& uri, int line, int character, JsonCallback cb);
    void requestHover(const QString& uri, int line, int character, JsonCallback cb);
    void requestSignatureHelp(const QString& uri, int line, int character, JsonCallback cb);
    void requestDefinition(const QString& uri, int line, int character, JsonCallback cb);

signals:
    void ready();
    void diagnosticsReceived(const QString& uri,
        const QList<miskeyed::workbench::slang_rhi::LspDiagnostic>& diagnostics);

private:
    void postNotification(const QString& method, const QJsonObject& params);
    void postRequest(const QString& method, const QJsonObject& params, JsonCallback cb);
    void sendRequestNow(const QString& method, const QJsonObject& params, JsonCallback cb);
    void writeMessage(const QJsonObject& message);
    void onReadyRead();
    void handleMessage(const QJsonObject& message);
    QJsonObject positionParams(const QString& uri, int line, int character) const;

    QProcess* m_proc = nullptr;
    QByteArray m_buffer;
    int m_nextId = 1;
    bool m_ready = false;
    QHash<int, JsonCallback> m_pending;
    QHash<QString, int> m_versions;
    QList<std::function<void()>> m_queued; // sent once the handshake completes
};

} // namespace miskeyed::workbench::slang_rhi

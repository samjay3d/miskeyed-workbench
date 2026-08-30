#pragma once

#include <miskeyed/workbench/Export.h>
#include <QObject>
#include <QString>

namespace miskeyed::workbench::slang_rhi {
class ShaderDocument;

// A Shader Toy contribution owns only its binding. The document, compiled artifact,
// time and editor state remain owned by the workspace and can have other consumers.
class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT ShaderToySession final : public QObject {
    Q_OBJECT
public:
    explicit ShaderToySession(QObject* parent = nullptr);

    ShaderDocument* shaderDocument() const { return m_shader; }
    QString vertexEntry() const { return m_vertexEntry; }
    QString fragmentEntry() const { return m_fragmentEntry; }
    bool canBindShader(ShaderDocument* document) const;
    bool bindShader(ShaderDocument* document);
    bool selectEntryPoints(const QString& vertex, const QString& fragment);
    void resolveEntryPoints();
    void removeDocument(ShaderDocument* document);

signals:
    void bindingChanged(ShaderDocument* document);
    void bindingRejected(ShaderDocument* document, const QString& reason);
    void entryPointsChanged(const QString& vertex, const QString& fragment);

private:
    ShaderDocument* m_shader = nullptr;
    QString m_vertexEntry;
    QString m_fragmentEntry;
};

} // namespace miskeyed::workbench::slang_rhi

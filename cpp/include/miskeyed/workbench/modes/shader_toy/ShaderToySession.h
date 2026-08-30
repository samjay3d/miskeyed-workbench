#pragma once

#include <miskeyed/workbench/Export.h>
#include <QObject>

namespace miskeyed::workbench::slang_rhi {
class ShaderDocument;

// A Shader Toy contribution owns only its binding. The document, compiled artifact,
// time and editor state remain owned by the workspace and can have other consumers.
class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT ShaderToySession final : public QObject {
    Q_OBJECT
public:
    explicit ShaderToySession(QObject* parent = nullptr);

    ShaderDocument* shaderDocument() const { return m_shader; }
    void bindShader(ShaderDocument* document);
    void removeDocument(ShaderDocument* document);

signals:
    void bindingChanged(ShaderDocument* document);

private:
    ShaderDocument* m_shader = nullptr;
};

} // namespace miskeyed::workbench::slang_rhi

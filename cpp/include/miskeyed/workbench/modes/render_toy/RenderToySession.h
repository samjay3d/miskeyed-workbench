#pragma once

#include <miskeyed/workbench/Export.h>
#include <QObject>

namespace miskeyed::workbench::core {
class TimeContext;
class TimeTransport;
}

namespace miskeyed::workbench::slang_rhi {
class ShaderDocument;

// Mode-owned runtime state. The generic workspace neither knows nor mutates these
// bindings; opening a document and using it in Render Toy are separate operations.
class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT RenderToySession final : public QObject {
    Q_OBJECT
public:
    explicit RenderToySession(QObject* parent = nullptr);

    ShaderDocument* sceneDocument() const { return m_scene; }
    ShaderDocument* postDocument() const { return m_post; }
    core::TimeContext* timeContext() const { return m_timeContext; }
    core::TimeTransport* timeTransport() const { return m_timeTransport; }

    void bindScene(ShaderDocument* document);
    void bindPost(ShaderDocument* document);
    void removeDocument(ShaderDocument* document);

signals:
    void bindingsChanged(ShaderDocument* scene, ShaderDocument* post);

private:
    ShaderDocument* m_scene = nullptr;
    ShaderDocument* m_post = nullptr;
    core::TimeContext* m_timeContext = nullptr;
    core::TimeTransport* m_timeTransport = nullptr;
};

} // namespace miskeyed::workbench::slang_rhi

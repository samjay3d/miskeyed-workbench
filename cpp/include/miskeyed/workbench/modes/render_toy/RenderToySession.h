#pragma once

#include <miskeyed/workbench/Export.h>
#include <QObject>

namespace miskeyed::workbench::core {
class TimeContext;
class TimeTransport;
}

namespace miskeyed::workbench::slang_rhi {
class ShaderDocument;

// Tool-owned runtime state. The workspace owns documents and evaluation time; this
// contribution only binds shared documents into Render Toy's two runtime slots.
class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT RenderToySession final : public QObject {
    Q_OBJECT
public:
    explicit RenderToySession(QObject* parent = nullptr);

    ShaderDocument* sceneDocument() const { return m_scene; }
    ShaderDocument* postDocument() const { return m_post; }
    core::TimeContext* timeContext() const { return m_timeContext; }
    core::TimeTransport* timeTransport() const { return m_timeTransport; }
    void setEvaluationContext(core::TimeContext* context, core::TimeTransport* transport);

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

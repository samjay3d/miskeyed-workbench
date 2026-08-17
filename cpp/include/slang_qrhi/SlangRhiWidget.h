#pragma once

#include "Export.h"
#include "ShaderDocument.h"
#include <QRhiWidget>
#include <QPointF>
#include <memory>

class QMouseEvent;
class QWheelEvent;

namespace slang_qrhi {

class SlangRhiWidgetPrivate;

class SLANG_QRHI_EXPORT SlangRhiWidget final : public QRhiWidget {
    Q_OBJECT
    Q_PROPERTY(ShaderDocument* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(float exposure READ exposure WRITE setExposure NOTIFY exposureChanged)

public:
    explicit SlangRhiWidget(QWidget* parent = nullptr);
    ~SlangRhiWidget() override;

    ShaderDocument* document() const { return m_document; }
    void setDocument(ShaderDocument* document);
    float exposure() const { return m_exposure; }
    void setExposure(float value);

    // Optional scene pre-pass. When set, this widget renders the scene document into an
    // offscreen color texture (the "G-buffer") first, then runs its own document as a
    // post-process that samples that texture (bound at SRB slot 1 -> HLSL t1/s1).
    ShaderDocument* scenePass() const { return m_scenePass; }
    void setScenePass(ShaderDocument* sceneDocument);

signals:
    void documentChanged();
    void exposureChanged();
    void gpuError(QString message);

protected:
    void initialize(QRhiCommandBuffer* cb) override;
    void render(QRhiCommandBuffer* cb) override;
    void releaseResources() override;

private slots:
    void onShaderChanged();
    void onParameterRangeChanged(int offset, int size);
    void onScenePassChanged();
    void onScenePassRangeChanged(int offset, int size);

private:
    // Houdini-style viewport navigation: LMB tumble, MMB pan, RMB/wheel dolly.
    // Drives the shader's `cam*` uniforms so any camera-aware shader responds.
    // Declared private so Shiboken skips them (QMouseEvent/QWheelEvent are not in
    // this module's binding); Qt still dispatches to them through the base virtuals.
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    bool nudgeParam(const char* name, float delta);

    ShaderDocument* m_document = nullptr;
    ShaderDocument* m_scenePass = nullptr;
    float m_exposure = 1.0f;
    QPointF m_lastDragPos;
    bool m_dragging = false;
    std::unique_ptr<SlangRhiWidgetPrivate> d;
};

} // namespace slang_qrhi

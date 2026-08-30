#pragma once

#include <miskeyed/workbench/core/DependencyGraph.h>
#include <miskeyed/workbench/slang/ShaderParameterModel.h>
#include <miskeyed/workbench/slang/SlangCompiler.h>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QMap>
#include <QStringList>

namespace miskeyed::workbench::slang_rhi {

class MISKEYED_WORKBENCH_SLANG_RHI_EXPORT ShaderDocument final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl fileUrl READ fileUrl WRITE setFileUrl NOTIFY fileUrlChanged)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool live READ live WRITE setLive NOTIFY liveChanged)
    Q_PROPERTY(bool compiling READ compiling NOTIFY compilingChanged)
    Q_PROPERTY(QString diagnostics READ diagnostics NOTIFY diagnosticsChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY dirtyChanged)
    Q_PROPERTY(QString dependencyIdentity READ dependencyIdentity NOTIFY compiled)
    Q_PROPERTY(ShaderParameterModel* parameters READ parameters CONSTANT)
    Q_PROPERTY(DependencyGraph* dependencyGraph READ dependencyGraph CONSTANT)

public:
    explicit ShaderDocument(QObject* parent = nullptr);

    QUrl fileUrl() const { return m_fileUrl; }
    void setFileUrl(const QUrl& url);
    QString source() const { return m_source; }
    void setSource(const QString& source);
    bool live() const { return m_live; }
    void setLive(bool live);
    bool compiling() const { return m_compiling; }
    QString diagnostics() const { return m_diagnostics; }
    bool dirty() const { return m_dirty; }
    QString dependencyIdentity() const { return m_graph.digestHex(m_pipelineNode); }

    ShaderParameterModel* parameters() { return &m_parameters; }
    DependencyGraph* dependencyGraph() { return &m_graph; }

    const QShader& vertexShader() const { return m_vertexShader; }
    const QShader& fragmentShader() const { return m_fragmentShader; }
    int parameterBinding() const { return m_parameterBinding; }

    // Wall-clock duration of the last compile in milliseconds (-1 if never compiled).
    int lastCompileMs() const { return m_lastCompileMs; }

    // Generated backend code from the last successful compile, for the editor's
    // "compiled output" viewer. `generatedTargets()` lists the available backends
    // (e.g. "HLSL", "GLSL", "SPIR-V", "Metal") in display order.
    QStringList generatedTargets() const { return m_generatedTargets; }
    QString generatedCode(const QString& target) const { return m_generated.value(target); }

    Q_INVOKABLE bool load();
    Q_INVOKABLE bool save();
    Q_INVOKABLE void compile();
    void markSourceClean();

signals:
    void fileUrlChanged();
    void sourceChanged();
    void liveChanged();
    void compilingChanged();
    void diagnosticsChanged();
    void compiled();
    void compileFailed(QString diagnostics);
    void shaderPackageChanged();
    void dirtyChanged();

private slots:
    void compileNow();

private:
    void setDiagnostics(QString text);
    void initializeGraph();

    QUrl m_fileUrl;
    QString m_source;
    bool m_live = true;
    bool m_compiling = false;
    bool m_dirty = false;
    QString m_diagnostics;
    QTimer m_compileTimer;
    SlangCompiler m_compiler;
    ShaderParameterModel m_parameters;
    DependencyGraph m_graph;
    QShader m_vertexShader;
    QShader m_fragmentShader;
    int m_parameterBinding = 0;
    int m_lastCompileMs = -1;
    QMap<QString, QString> m_generated;
    QStringList m_generatedTargets;
    NodeId m_sourceNode = 0;
    NodeId m_entryNode = 0;
    NodeId m_uiNode = 0;
    NodeId m_layoutNode = 0;
    NodeId m_valuesNode = 0;
    NodeId m_pipelineNode = 0;
};

} // namespace miskeyed::workbench::slang_rhi

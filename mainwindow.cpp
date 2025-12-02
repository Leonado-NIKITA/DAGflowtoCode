#include "MainWindow.h"
#include "NodeScene.h"
#include "NodeView.h"
#include "Node.h"
#include "Connection.h"
#include "GroupNode.h"
#include "CodeGenerator.h"
#include "NodeLibrary.h"
#include "NodeTemplate.h"
#include "NodeEditDialog.h"
#include "DraggableNodeTree.h"

#include <QDockWidget>
#include <QTabWidget>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QSpinBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
#include <QFormLayout>
#include <functional>
#include <QTextStream>
#include <QStringConverter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_scene(new NodeScene(this))
    , m_view(new NodeView(m_scene, this))
{
    setupUI();
    createMenus();
    createToolBars();
    createDockWidgets();
    setupConnections();
    
    // 从节点库初始化模板映射
    refreshNodeLibrary();
    
    // 连接节点库变化信号
    connect(NodeLibrary::instance(), &NodeLibrary::libraryChanged,
            this, &MainWindow::refreshNodeLibrary);
    
    setWindowTitle("Qt节点编辑器");
    setMinimumSize(1200, 800);
    resize(1400, 900);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setCentralWidget(m_view);
    
    // 状态栏
    statusBar()->showMessage("就绪");
}

void MainWindow::createMenus()
{
    // 文件菜单
    QMenu *fileMenu = menuBar()->addMenu("文件");
    fileMenu->addAction("新建项目", this, &MainWindow::onClearCanvas, QKeySequence::New);
    fileMenu->addAction("打开项目", this, &MainWindow::onLoadProject, QKeySequence::Open);
    fileMenu->addAction("保存项目", this, &MainWindow::onSaveProject, QKeySequence::Save);
    fileMenu->addSeparator();
    fileMenu->addAction("退出", this, &QApplication::exec, QKeySequence::Quit);
    
    // 编辑菜单
    QMenu *editMenu = menuBar()->addMenu("编辑");
    editMenu->addAction("撤销", [this]() { m_scene->undoStack()->undo(); }, QKeySequence::Undo);
    editMenu->addAction("重做", [this]() { m_scene->undoStack()->redo(); }, QKeySequence::Redo);
    editMenu->addSeparator();
    editMenu->addAction("复制", [this]() { m_scene->copySelected(); }, QKeySequence::Copy);
    editMenu->addAction("剪切", [this]() { m_scene->cutSelected(); }, QKeySequence::Cut);
    editMenu->addAction("粘贴", [this]() {
        if (m_scene->canPaste()) {
            QPointF center = m_view->mapToScene(m_view->viewport()->rect().center());
            m_scene->paste(center);
        }
    }, QKeySequence::Paste);
    editMenu->addSeparator();
    editMenu->addAction("全选", [this]() { m_scene->selectAll(); }, QKeySequence::SelectAll);
    editMenu->addAction("删除选中", this, &MainWindow::onDeleteSelected, QKeySequence::Delete);
    editMenu->addSeparator();
    editMenu->addAction("打包节点", [this]() {
        if (m_scene->canGroup()) {
            m_scene->groupSelected();
        } else {
            statusBar()->showMessage("请选中至少两个节点进行打包");
        }
    }, QKeySequence(Qt::CTRL | Qt::Key_G));
    editMenu->addAction("拆分节点", [this]() {
        if (m_scene->canUngroup()) {
            m_scene->ungroupSelected();
        } else {
            statusBar()->showMessage("请选中一个组合节点进行拆分");
        }
    }, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G));
    editMenu->addSeparator();
    editMenu->addAction("清空画布", this, &MainWindow::onClearCanvas);
    
    // 节点库菜单
    QMenu *nodeLibraryMenu = menuBar()->addMenu("节点库");
    nodeLibraryMenu->addAction("创建自定义节点...", this, &MainWindow::onCreateCustomNode);
    nodeLibraryMenu->addAction("编辑节点模板...", this, &MainWindow::onEditNodeTemplate);
    nodeLibraryMenu->addAction("删除自定义节点", this, &MainWindow::onDeleteNodeTemplate);
    nodeLibraryMenu->addSeparator();
    nodeLibraryMenu->addAction("导入节点库...", this, &MainWindow::onLoadNodeLibrary);
    nodeLibraryMenu->addAction("导出节点库...", this, &MainWindow::onSaveNodeLibrary);
    nodeLibraryMenu->addSeparator();
    nodeLibraryMenu->addAction("刷新节点库", this, &MainWindow::refreshNodeLibrary);

    // 生成菜单
    QMenu *generateMenu = menuBar()->addMenu("生成");
    generateMenu->addAction("验证流程", [this]() {
        if (m_scene->validateFlow()) {
            statusBar()->showMessage("流程验证通过");
        } else {
            QMessageBox::warning(this, "验证失败", "流程图存在错误");
        }
    });
    generateMenu->addAction("生成代码", this, &MainWindow::onGenerateCode);
    generateMenu->addSeparator();
    
    // 导出子菜单
    QMenu *exportMenu = generateMenu->addMenu("导出代码");
    exportMenu->addAction("导出为 JSON...", this, &MainWindow::onExportCodeAsJson);
    exportMenu->addAction("导出为 Python...", this, &MainWindow::onExportCodeAsPython);
    exportMenu->addAction("导出为 YAML...", this, &MainWindow::onExportCodeAsYaml);
    
    // 帮助菜单
    QMenu *helpMenu = menuBar()->addMenu("帮助");
    helpMenu->addAction("关于", []() {
        QMessageBox::about(qApp->activeWindow(), "关于", 
            "Qt节点编辑器 v1.0\n基于Qt6的可视化节点编辑工具");
    });
}

void MainWindow::createToolBars()
{
    QToolBar *mainToolBar = addToolBar("主工具栏");
    
    mainToolBar->addAction("新建", this, &MainWindow::onClearCanvas);
    mainToolBar->addAction("保存", this, &MainWindow::onSaveProject);
    mainToolBar->addAction("加载", this, &MainWindow::onLoadProject);
    mainToolBar->addSeparator();
    mainToolBar->addAction("生成代码", this, &MainWindow::onGenerateCode);
    mainToolBar->addSeparator();
    mainToolBar->addAction("导出JSON", this, &MainWindow::onExportCodeAsJson);
    mainToolBar->addAction("导出Python", this, &MainWindow::onExportCodeAsPython);
}

void MainWindow::createDockWidgets()
{
    // 节点库停靠窗口
    QDockWidget *nodeLibraryDock = new QDockWidget("节点库", this);
    
    // 创建节点库容器
    QWidget *nodeLibraryWidget = new QWidget();
    QVBoxLayout *nodeLibraryLayout = new QVBoxLayout(nodeLibraryWidget);
    nodeLibraryLayout->setContentsMargins(5, 5, 5, 5);
    
    m_nodeLibrary = new DraggableNodeTree();
    m_nodeLibrary->setHeaderLabel("可用节点");
    m_nodeLibrary->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // 节点库将在refreshNodeLibrary()中填充
    // 拖拽功能已在 DraggableNodeTree 中启用
    
    // 添加节点库管理按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *addNodeBtn = new QPushButton("添加节点");
    QPushButton *editNodeBtn = new QPushButton("编辑");
    QPushButton *deleteNodeBtn = new QPushButton("删除");
    
    addNodeBtn->setToolTip("创建新的自定义节点");
    editNodeBtn->setToolTip("编辑选中的自定义节点模板");
    deleteNodeBtn->setToolTip("删除选中的自定义节点模板");
    
    connect(addNodeBtn, &QPushButton::clicked, this, &MainWindow::onCreateCustomNode);
    connect(editNodeBtn, &QPushButton::clicked, this, &MainWindow::onEditNodeTemplate);
    connect(deleteNodeBtn, &QPushButton::clicked, this, &MainWindow::onDeleteNodeTemplate);
    
    buttonLayout->addWidget(addNodeBtn);
    buttonLayout->addWidget(editNodeBtn);
    buttonLayout->addWidget(deleteNodeBtn);
    
    nodeLibraryLayout->addWidget(m_nodeLibrary);
    nodeLibraryLayout->addLayout(buttonLayout);
    
    nodeLibraryDock->setWidget(nodeLibraryWidget);
    addDockWidget(Qt::LeftDockWidgetArea, nodeLibraryDock);
    
    // 场景节点树停靠窗口
    QDockWidget *sceneNodeDock = new QDockWidget("场景节点", this);
    QWidget *sceneNodeWidget = new QWidget();
    QVBoxLayout *sceneNodeLayout = new QVBoxLayout(sceneNodeWidget);
    sceneNodeLayout->setContentsMargins(5, 5, 5, 5);
    
    m_sceneNodeTree = new QTreeWidget();
    m_sceneNodeTree->setHeaderLabel("绘图中的节点");
    m_sceneNodeTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_sceneNodeTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_sceneNodeTree->setAnimated(true);
    m_sceneNodeTree->setExpandsOnDoubleClick(true);
    
    // 连接点击信号
    connect(m_sceneNodeTree, &QTreeWidget::itemClicked, 
            this, &MainWindow::onSceneNodeTreeItemClicked);
    
    sceneNodeLayout->addWidget(m_sceneNodeTree);
    sceneNodeDock->setWidget(sceneNodeWidget);
    addDockWidget(Qt::LeftDockWidgetArea, sceneNodeDock);
    
    // 将两个左侧停靠窗口堆叠
    tabifyDockWidget(nodeLibraryDock, sceneNodeDock);
    nodeLibraryDock->raise();  // 默认显示节点库
    
    // 设置左侧停靠区域的标签位置为上方
    setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);
    
    // 属性编辑停靠窗口
    QDockWidget *propertyDock = new QDockWidget("属性编辑器", this);
    QWidget *propertyWidget = new QWidget();
    QVBoxLayout *propertyLayout = new QVBoxLayout(propertyWidget);
    
    // ========== 节点属性编辑 ==========
    m_nodePropsWidget = new QWidget();
    QVBoxLayout *nodePropsLayout = new QVBoxLayout(m_nodePropsWidget);
    nodePropsLayout->setContentsMargins(0, 0, 0, 0);
    
    QGroupBox *nodePropsGroup = new QGroupBox("节点属性");
    QFormLayout *formLayout = new QFormLayout();
    
    m_nodeNameEdit = new QLineEdit();
    m_nodeTypeCombo = new QComboBox();
    m_nodeParamsEdit = new QLineEdit();
    m_updatePropsButton = new QPushButton("更新属性");
    
    // 填充节点类型
    for (auto it = m_nodeTemplates.begin(); it != m_nodeTemplates.end(); ++it) {
        m_nodeTypeCombo->addItem(it.value(), it.key());
    }
    
    formLayout->addRow("名称:", m_nodeNameEdit);
    formLayout->addRow("类型:", m_nodeTypeCombo);
    formLayout->addRow("参数:", m_nodeParamsEdit);
    
    nodePropsGroup->setLayout(formLayout);
    
    nodePropsLayout->addWidget(nodePropsGroup);
    nodePropsLayout->addWidget(m_updatePropsButton);
    
    // 端口连接关系显示
    QGroupBox *connectionGroup = new QGroupBox("端口连接关系");
    QVBoxLayout *connectionLayout = new QVBoxLayout();
    
    m_connectionTree = new QTreeWidget();
    m_connectionTree->setHeaderLabels({"端口", "连接到", "目标端口"});
    m_connectionTree->setColumnCount(3);
    m_connectionTree->setAlternatingRowColors(true);
    m_connectionTree->setRootIsDecorated(true);
    m_connectionTree->header()->setStretchLastSection(true);
    m_connectionTree->setMinimumHeight(150);
    
    connectionLayout->addWidget(m_connectionTree);
    connectionGroup->setLayout(connectionLayout);
    
    nodePropsLayout->addWidget(connectionGroup);
    
    propertyLayout->addWidget(m_nodePropsWidget);
    
    // ========== 连线属性编辑 ==========
    m_connPropsWidget = new QWidget();
    QVBoxLayout *connPropsLayout = new QVBoxLayout(m_connPropsWidget);
    connPropsLayout->setContentsMargins(0, 0, 0, 0);
    
    QGroupBox *connPropsGroup = new QGroupBox("连线属性");
    QFormLayout *connFormLayout = new QFormLayout();
    
    m_connFromNodeEdit = new QLineEdit();
    m_connFromNodeEdit->setReadOnly(true);
    m_connToNodeEdit = new QLineEdit();
    m_connToNodeEdit->setReadOnly(true);
    m_connFromPortEdit = new QLineEdit();
    m_connFromPortEdit->setReadOnly(true);
    m_connToPortEdit = new QLineEdit();
    m_connToPortEdit->setReadOnly(true);
    m_connLineTypeCombo = new QComboBox();
    
    // 填充连线类型
    m_connLineTypeCombo->addItem("贝塞尔曲线", 0);
    m_connLineTypeCombo->addItem("直线", 1);
    m_connLineTypeCombo->addItem("直角线", 2);
    
    connFormLayout->addRow("源节点:", m_connFromNodeEdit);
    connFormLayout->addRow("源端口:", m_connFromPortEdit);
    connFormLayout->addRow("目标节点:", m_connToNodeEdit);
    connFormLayout->addRow("目标端口:", m_connToPortEdit);
    connFormLayout->addRow("连线类型:", m_connLineTypeCombo);
    
    connPropsGroup->setLayout(connFormLayout);
    connPropsLayout->addWidget(connPropsGroup);
    
    // 连线类型改变信号连接
    connect(m_connLineTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onConnectionLineTypeChanged);
    
    propertyLayout->addWidget(m_connPropsWidget);
    m_connPropsWidget->hide();  // 默认隐藏连线属性
    
    // ========== 组节点属性编辑 ==========
    m_groupPropsWidget = new QWidget();
    QVBoxLayout *groupPropsLayout = new QVBoxLayout(m_groupPropsWidget);
    groupPropsLayout->setContentsMargins(0, 0, 0, 0);
    
    QGroupBox *groupPropsGroup = new QGroupBox("组合节点属性");
    QFormLayout *groupFormLayout = new QFormLayout();
    
    m_groupLevelSpinBox = new QSpinBox();
    m_groupLevelSpinBox->setRange(1, 99);
    m_groupLevelSpinBox->setValue(1);
    m_groupLevelSpinBox->setToolTip("设置组件等级 (1-99)");
    
    groupFormLayout->addRow("组件等级:", m_groupLevelSpinBox);
    
    groupPropsGroup->setLayout(groupFormLayout);
    groupPropsLayout->addWidget(groupPropsGroup);
    
    // 组件等级改变信号连接
    connect(m_groupLevelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int value) {
                // 获取当前选中的组节点
                QGraphicsItem *item = m_scene->getSelectedNode();
                if (GroupNode *groupNode = dynamic_cast<GroupNode*>(item)) {
                    groupNode->setGroupLevel(value);
                    updateSceneNodeTree();  // 更新场景节点树显示
                }
            });
    
    propertyLayout->addWidget(m_groupPropsWidget);
    m_groupPropsWidget->hide();  // 默认隐藏组节点属性
    
    propertyLayout->addStretch();
    
    propertyDock->setWidget(propertyWidget);
    addDockWidget(Qt::RightDockWidgetArea, propertyDock);
    
    // 初始化选中连线指针
    m_selectedConnection = nullptr;
    
    // 代码输出停靠窗口
    QDockWidget *codeDock = new QDockWidget("代码生成", this);
    m_codeOutput = new QTextEdit();
    m_codeOutput->setFontFamily("Courier New");
    m_codeOutput->setReadOnly(true);
    codeDock->setWidget(m_codeOutput);
    addDockWidget(Qt::BottomDockWidgetArea, codeDock);
}

void MainWindow::setupConnections()
{
    connect(m_nodeLibrary, &QTreeWidget::itemDoubleClicked, [this](QTreeWidgetItem *item, int column) {
        Q_UNUSED(column);
        if (item->childCount() == 0) { // 只有叶子节点可添加
            QString nodeType = item->data(0, Qt::UserRole).toString();
            if (!nodeType.isEmpty()) {
                // 计算新节点的位置（视图中心）
                QPointF center = m_view->mapToScene(m_view->viewport()->rect().center());
                m_scene->addNode(nodeType, center);
                statusBar()->showMessage(QString("添加节点: %1").arg(item->text(0)));
            }
        }
    });
    
    connect(m_scene, &NodeScene::selectionChanged, this, &MainWindow::onNodeSelected);
    connect(m_scene, &NodeScene::connectionCreated, this, &MainWindow::onConnectionCreated);
    
    // 场景变化时更新节点树
    connect(m_scene, &QGraphicsScene::changed, this, &MainWindow::updateSceneNodeTree);
    
    connect(m_updatePropsButton, &QPushButton::clicked, this, &MainWindow::onUpdateNodeProperties);
}

void MainWindow::onNodeSelected(QGraphicsItem* item)
{
    // 清空连接关系树
    m_connectionTree->clear();
    m_selectedConnection = nullptr;
    
    // 检查是否选中了连线
    if (Connection *conn = dynamic_cast<Connection*>(item)) {
        // 显示连线属性，隐藏节点属性
        m_nodePropsWidget->hide();
        m_connPropsWidget->show();
        m_selectedConnection = conn;
        
        // 填充连线属性
        Node *fromNode = conn->getFromNode();
        Node *toNode = conn->getToNode();
        
        m_connFromNodeEdit->setText(fromNode ? fromNode->getName() : "未知");
        m_connToNodeEdit->setText(toNode ? toNode->getName() : "未知");
        m_connFromPortEdit->setText(QString("输出 %1").arg(conn->getFromPortIndex()));
        m_connToPortEdit->setText(QString("输入 %1").arg(conn->getToPortIndex()));
        
        // 设置连线类型（阻止信号触发）
        m_connLineTypeCombo->blockSignals(true);
        m_connLineTypeCombo->setCurrentIndex(static_cast<int>(conn->getLineType()));
        m_connLineTypeCombo->blockSignals(false);
        
        statusBar()->showMessage(QString("选中连线: %1 -> %2")
            .arg(fromNode ? fromNode->getName() : "?")
            .arg(toNode ? toNode->getName() : "?"));
        return;
    }
    
    // 检查是否选中了节点
    if (Node *node = dynamic_cast<Node*>(item)) {
        // 显示节点属性，隐藏连线属性
        m_nodePropsWidget->show();
        m_connPropsWidget->hide();
        
        m_nodeNameEdit->setText(node->getName());
        m_nodeTypeCombo->setCurrentText(m_nodeTemplates.value(node->getType()));
        m_nodeParamsEdit->setText(node->getParameters().join(", "));
        
        // 检查是否为组节点，显示/隐藏组节点属性
        if (GroupNode *groupNode = dynamic_cast<GroupNode*>(node)) {
            m_groupPropsWidget->show();
            m_groupLevelSpinBox->blockSignals(true);
            m_groupLevelSpinBox->setValue(groupNode->getGroupLevel());
            m_groupLevelSpinBox->blockSignals(false);
        } else {
            m_groupPropsWidget->hide();
        }
        
        // 显示端口连接关系
        QList<Connection*> connections = node->getConnections();
        
        // 创建输入端口分组
        if (node->getInputPortCount() > 0) {
            QTreeWidgetItem *inputGroup = new QTreeWidgetItem(m_connectionTree);
            inputGroup->setText(0, QString("输入端口 (%1个)").arg(node->getInputPortCount()));
            inputGroup->setExpanded(true);
            
            // 为每个输入端口创建条目
            for (int i = 0; i < node->getInputPortCount(); ++i) {
                QTreeWidgetItem *portItem = new QTreeWidgetItem(inputGroup);
                portItem->setText(0, QString("输入 %1").arg(i));
                
                // 查找连接到此输入端口的连接
                bool hasConnection = false;
                for (Connection *conn : connections) {
                    if (conn->getToNode() == node && conn->getToPortIndex() == i) {
                        Node *sourceNode = conn->getFromNode();
                        portItem->setText(1, sourceNode ? sourceNode->getName() : "未知");
                        portItem->setText(2, QString("输出 %1").arg(conn->getFromPortIndex()));
                        portItem->setForeground(1, QBrush(QColor(100, 200, 100)));
                        hasConnection = true;
                        break;
                    }
                }
                
                if (!hasConnection) {
                    portItem->setText(1, "未连接");
                    portItem->setForeground(1, QBrush(QColor(150, 150, 150)));
                }
            }
        }
        
        // 创建输出端口分组
        if (node->getOutputPortCount() > 0) {
            QTreeWidgetItem *outputGroup = new QTreeWidgetItem(m_connectionTree);
            outputGroup->setText(0, QString("输出端口 (%1个)").arg(node->getOutputPortCount()));
            outputGroup->setExpanded(true);
            
            // 为每个输出端口创建条目
            for (int i = 0; i < node->getOutputPortCount(); ++i) {
                QTreeWidgetItem *portItem = new QTreeWidgetItem(outputGroup);
                portItem->setText(0, QString("输出 %1").arg(i));
                
                // 查找从此输出端口发出的连接
                QStringList connectedNodes;
                QStringList connectedPorts;
                for (Connection *conn : connections) {
                    if (conn->getFromNode() == node && conn->getFromPortIndex() == i) {
                        Node *targetNode = conn->getToNode();
                        if (targetNode) {
                            connectedNodes.append(targetNode->getName());
                            connectedPorts.append(QString("输入 %1").arg(conn->getToPortIndex()));
                        }
                    }
                }
                
                if (!connectedNodes.isEmpty()) {
                    portItem->setText(1, connectedNodes.join(", "));
                    portItem->setText(2, connectedPorts.join(", "));
                    portItem->setForeground(1, QBrush(QColor(100, 200, 100)));
                } else {
                    portItem->setText(1, "未连接");
                    portItem->setForeground(1, QBrush(QColor(150, 150, 150)));
                }
            }
        }
        
        // 调整列宽
        m_connectionTree->resizeColumnToContents(0);
        m_connectionTree->resizeColumnToContents(1);
        
        statusBar()->showMessage(QString("选中节点: %1 (连接数: %2)")
            .arg(node->getName()).arg(connections.size()));
    } else {
        // 未选中任何有效项
        m_nodePropsWidget->show();
        m_connPropsWidget->hide();
        m_nodeNameEdit->clear();
        m_nodeParamsEdit->clear();
    }
}

/**
 * @brief 连线类型改变槽函数
 * @param index 新的类型索引
 */
void MainWindow::onConnectionLineTypeChanged(int index)
{
    if (m_selectedConnection) {
        m_selectedConnection->setLineType(static_cast<Connection::LineType>(index));
        statusBar()->showMessage(QString("连线类型已更改为: %1")
            .arg(Connection::lineTypeName(static_cast<Connection::LineType>(index))));
    }
}

/**
 * @brief 更新连线属性槽函数
 */
void MainWindow::onUpdateConnectionProperties()
{
    // 预留用于未来扩展连线属性编辑功能
    if (m_selectedConnection) {
        statusBar()->showMessage("连线属性已更新");
    }
}

void MainWindow::onConnectionCreated()
{
    statusBar()->showMessage("连接创建成功");
}

void MainWindow::onGenerateCode()
{
    CodeGenerator generator;
    QString code = generator.generateCode(m_scene->getFlowData());
    m_codeOutput->setPlainText(code);
    statusBar()->showMessage("代码生成完成");
}

void MainWindow::onSaveProject()
{
    QString fileName = QFileDialog::getSaveFileName(this, "保存项目", "", "节点项目文件 (*.json)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            QJsonObject flowData = m_scene->getFlowData();
            QJsonDocument doc(flowData);
            file.write(doc.toJson());
            statusBar()->showMessage("项目保存成功");
        }
    }
}

void MainWindow::onLoadProject()
{
    QString fileName = QFileDialog::getOpenFileName(this, "打开项目", "", "节点项目文件 (*.json)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            m_scene->loadFlowData(doc.object());
            statusBar()->showMessage("项目加载成功");
        }
    }
}

void MainWindow::onClearCanvas()
{
    if (QMessageBox::question(this, "确认", "确定要清空画布吗？") == QMessageBox::Yes) {
        m_scene->clear();
        m_codeOutput->clear();
        statusBar()->showMessage("画布已清空");
    }
}

void MainWindow::onAddNode()
{
    // 在场景中心添加默认节点
    QPointF center = m_view->mapToScene(m_view->viewport()->rect().center());
    m_scene->addNode("signal_source", center);
}

void MainWindow::onUpdateNodeProperties()
{
    if (Node *selectedNode = dynamic_cast<Node*>(m_scene->getSelectedNode())) {
        selectedNode->setName(m_nodeNameEdit->text());
        selectedNode->setType(m_nodeTypeCombo->currentData().toString());
        selectedNode->setParameters(m_nodeParamsEdit->text().split(',', Qt::SkipEmptyParts));
        m_scene->update();
        statusBar()->showMessage("节点属性已更新");
    }
}

void MainWindow::onDeleteSelected()
{
    m_scene->deleteSelected();
    statusBar()->showMessage("选中项已删除");
}

void MainWindow::onCreateCustomNode()
{
    NodeEditDialog dialog(NodeEditDialog::CreateMode, this);
    if (dialog.exec() == QDialog::Accepted) {
        NodeTemplate tmpl = dialog.getTemplate();
        if (NodeLibrary::instance()->addTemplate(tmpl)) {
            // 节点库会自动保存
            statusBar()->showMessage(QString("节点 '%1' 创建成功").arg(tmpl.getDisplayName()));
        } else {
            QMessageBox::warning(this, "创建失败", "无法添加节点模板，类型ID可能已存在");
        }
    }
}

void MainWindow::onEditNodeTemplate()
{
    // 获取当前选中的节点模板
    QTreeWidgetItem *currentItem = m_nodeLibrary->currentItem();
    if (!currentItem || currentItem->childCount() > 0) {
        QMessageBox::information(this, "提示", "请在节点库中选择要编辑的节点");
        return;
    }
    
    QString typeId = currentItem->data(0, Qt::UserRole).toString();
    NodeTemplate tmpl = NodeLibrary::instance()->getTemplate(typeId);
    
    if (!tmpl.isValid()) {
        QMessageBox::warning(this, "错误", "无法获取节点模板信息");
        return;
    }
    
    NodeEditDialog dialog(NodeEditDialog::EditMode, this);
    dialog.setTemplate(tmpl);
    
    if (dialog.exec() == QDialog::Accepted) {
        NodeTemplate updatedTmpl = dialog.getTemplate();
        if (NodeLibrary::instance()->updateTemplate(updatedTmpl)) {
            // 节点库会自动保存
            statusBar()->showMessage(QString("节点模板 '%1' 更新成功").arg(updatedTmpl.getDisplayName()));
        } else {
            QMessageBox::warning(this, "更新失败", "无法更新节点模板");
        }
    }
}

void MainWindow::onDeleteNodeTemplate()
{
    // 获取当前选中的节点模板
    QTreeWidgetItem *currentItem = m_nodeLibrary->currentItem();
    if (!currentItem || currentItem->childCount() > 0) {
        QMessageBox::information(this, "提示", "请在节点库中选择要删除的节点");
        return;
    }
    
    QString typeId = currentItem->data(0, Qt::UserRole).toString();
    NodeTemplate tmpl = NodeLibrary::instance()->getTemplate(typeId);
    
    if (!tmpl.isValid()) {
        QMessageBox::warning(this, "错误", "无法获取节点模板信息");
        return;
    }
    
    if (QMessageBox::question(this, "确认删除",
            QString("确定要删除节点 '%1' 吗？").arg(tmpl.getDisplayName()))
            == QMessageBox::Yes) {
        if (NodeLibrary::instance()->removeTemplate(typeId)) {
            // 节点库会自动保存
            statusBar()->showMessage(QString("节点模板 '%1' 已删除").arg(tmpl.getDisplayName()));
        } else {
            QMessageBox::warning(this, "删除失败", "无法删除节点模板");
        }
    }
}

void MainWindow::onSaveNodeLibrary()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出节点库",
        "", "节点库文件 (*.nodelib.json);;所有文件 (*)");
    
    if (!fileName.isEmpty()) {
        if (!fileName.endsWith(".nodelib.json")) {
            fileName += ".nodelib.json";
        }
        
        if (NodeLibrary::instance()->saveToFile(fileName)) {
            statusBar()->showMessage(QString("节点库已导出到 %1").arg(fileName));
        } else {
            QMessageBox::warning(this, "导出失败", "无法保存节点库文件");
        }
    }
}

void MainWindow::onLoadNodeLibrary()
{
    QString fileName = QFileDialog::getOpenFileName(this, "导入节点库",
        "", "节点库文件 (*.nodelib.json);;JSON文件 (*.json);;所有文件 (*)");
    
    if (!fileName.isEmpty()) {
        if (NodeLibrary::instance()->loadFromFile(fileName)) {
            // 自动保存到默认路径以保持同步
            NodeLibrary::instance()->saveToFile(
                NodeLibrary::instance()->getDefaultLibraryPath());
            statusBar()->showMessage(QString("节点库已从 %1 导入").arg(fileName));
        } else {
            QMessageBox::warning(this, "导入失败", "无法加载节点库文件");
        }
    }
}

void MainWindow::refreshNodeLibrary()
{
    // 清空现有内容
    m_nodeLibrary->clear();
    m_nodeTemplates.clear();
    
    // 从NodeLibrary获取所有分类
    QStringList categories = NodeLibrary::instance()->getCategories();
    
    // 创建分类节点
    QMap<QString, QTreeWidgetItem*> categoryItems;
    for (const QString &category : categories) {
        QTreeWidgetItem *categoryItem = new QTreeWidgetItem(m_nodeLibrary, {category});
        categoryItem->setExpanded(true);
        categoryItems[category] = categoryItem;
    }
    
    // 添加节点模板
    QList<NodeTemplate> templates = NodeLibrary::instance()->getAllTemplates();
    for (const NodeTemplate &tmpl : templates) {
        QTreeWidgetItem *parentItem = categoryItems.value(tmpl.getCategory());
        if (!parentItem) {
            // 如果分类不存在，创建一个
            parentItem = new QTreeWidgetItem(m_nodeLibrary, {tmpl.getCategory()});
            parentItem->setExpanded(true);
            categoryItems[tmpl.getCategory()] = parentItem;
        }
        
        QTreeWidgetItem *nodeItem = new QTreeWidgetItem(parentItem, {tmpl.getDisplayName()});
        nodeItem->setData(0, Qt::UserRole, tmpl.getTypeId());
        
        // 设置节点颜色图标
        QPixmap pixmap(16, 16);
        pixmap.fill(tmpl.getColor());
        nodeItem->setIcon(0, QIcon(pixmap));
        
        // 设置提示文本
        QString tooltip = QString("类型: %1\n描述: %2")
            .arg(tmpl.getTypeId())
            .arg(tmpl.getDescription().isEmpty() ? "无" : tmpl.getDescription());
        nodeItem->setToolTip(0, tooltip);
        
        // 更新模板映射
        m_nodeTemplates[tmpl.getTypeId()] = tmpl.getDisplayName();
    }
    
    m_nodeLibrary->expandAll();
    
    // 更新属性编辑器中的节点类型下拉框
    if (m_nodeTypeCombo) {
        QString currentType = m_nodeTypeCombo->currentData().toString();
        m_nodeTypeCombo->clear();
        for (auto it = m_nodeTemplates.begin(); it != m_nodeTemplates.end(); ++it) {
            m_nodeTypeCombo->addItem(it.value(), it.key());
        }
        // 尝试恢复之前选中的类型
        int index = m_nodeTypeCombo->findData(currentType);
        if (index >= 0) {
            m_nodeTypeCombo->setCurrentIndex(index);
        }
    }
    
    statusBar()->showMessage("节点库已刷新");
}

void MainWindow::onExportCodeAsJson()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出JSON代码",
        "", "JSON文件 (*.json);;所有文件 (*)");
    
    if (!fileName.isEmpty()) {
        if (!fileName.endsWith(".json")) {
            fileName += ".json";
        }
        
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            CodeGenerator generator;
            QString code = generator.generateCode(m_scene->getFlowData());
            QTextStream stream(&file);
            stream.setEncoding(QStringConverter::Utf8);
            stream << code;
            file.close();
            statusBar()->showMessage(QString("代码已导出到 %1").arg(fileName));
        } else {
            QMessageBox::warning(this, "导出失败", "无法创建文件");
        }
    }
}

void MainWindow::onExportCodeAsPython()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出Python代码",
        "", "Python文件 (*.py);;所有文件 (*)");
    
    if (!fileName.isEmpty()) {
        if (!fileName.endsWith(".py")) {
            fileName += ".py";
        }
        
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            CodeGenerator generator;
            QString code = generator.generatePythonCode(m_scene->getFlowData());
            QTextStream stream(&file);
            stream.setEncoding(QStringConverter::Utf8);
            stream << code;
            file.close();
            statusBar()->showMessage(QString("Python代码已导出到 %1").arg(fileName));
        } else {
            QMessageBox::warning(this, "导出失败", "无法创建文件");
        }
    }
}

void MainWindow::onExportCodeAsYaml()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出YAML配置",
        "", "YAML文件 (*.yaml *.yml);;所有文件 (*)");
    
    if (!fileName.isEmpty()) {
        if (!fileName.endsWith(".yaml") && !fileName.endsWith(".yml")) {
            fileName += ".yaml";
        }
        
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            CodeGenerator generator;
            QString code = generator.generateConfigFile(m_scene->getFlowData());
            QTextStream stream(&file);
            stream.setEncoding(QStringConverter::Utf8);
            stream << code;
            file.close();
            statusBar()->showMessage(QString("YAML配置已导出到 %1").arg(fileName));
        } else {
            QMessageBox::warning(this, "导出失败", "无法创建文件");
        }
    }
}

// 辅助函数：递归添加节点到树
static void addNodeToTree(QTreeWidgetItem *parentItem, Node *node, int depth, QSet<QString> &expandedItems)
{
    QTreeWidgetItem *nodeItem = new QTreeWidgetItem();
    
    // 缩进前缀
    QString indent = "";
    for (int i = 0; i < depth; ++i) {
        indent += "  ";
    }
    if (depth > 0) {
        indent += "├─ ";
    }
    
    // 检查是否为组节点
    GroupNode *groupNode = dynamic_cast<GroupNode*>(node);
    
    if (groupNode) {
        // 组节点 - 使用特殊图标和样式，显示等级
        QString displayName = QString("%1📦 %2 [Lv.%3]").arg(indent).arg(node->getName()).arg(groupNode->getGroupLevel());
        nodeItem->setText(0, displayName);
        nodeItem->setToolTip(0, QString("组合节点: %1\n组件等级: %2\n包含 %3 个内部节点\n嵌套深度: %4")
                            .arg(node->getName())
                            .arg(groupNode->getGroupLevel())
                            .arg(groupNode->getInternalNodes().size())
                            .arg(depth));
        nodeItem->setForeground(0, QColor(100, 149, 237));  // 蓝色
        
        // 递归添加内部节点作为子项
        for (Node *internalNode : groupNode->getInternalNodes()) {
            addNodeToTree(nodeItem, internalNode, depth + 1, expandedItems);
        }
        
        // 恢复展开状态
        if (expandedItems.contains(displayName)) {
            nodeItem->setExpanded(true);
        }
    } else {
        // 普通节点
        QString displayType = node->getDisplayTypeName();
        if (displayType.isEmpty()) {
            displayType = node->getType();
        }
        nodeItem->setText(0, QString("%1● %2").arg(indent).arg(node->getName()));
        nodeItem->setToolTip(0, QString("节点: %1\n类型: %2")
                            .arg(node->getName())
                            .arg(displayType));
        
        // 根据深度设置颜色
        if (depth == 0) {
            nodeItem->setForeground(0, QColor(81, 207, 102));  // 绿色（顶层）
        } else {
            nodeItem->setForeground(0, QColor(150, 180, 150));  // 浅绿色（内部）
        }
    }
    
    // 存储节点指针
    nodeItem->setData(0, Qt::UserRole, reinterpret_cast<quintptr>(node));
    nodeItem->setData(0, Qt::UserRole + 1, depth > 0);  // 标记是否为内部节点
    
    if (parentItem) {
        parentItem->addChild(nodeItem);
    }
}

/**
 * @brief 更新场景节点树显示
 * 
 * 遍历场景中的所有节点，构建树形视图
 * 支持多层嵌套的组节点递归显示
 */
void MainWindow::updateSceneNodeTree()
{
    // 保存当前展开状态（递归收集）
    QSet<QString> expandedItems;
    std::function<void(QTreeWidgetItem*)> collectExpanded = [&](QTreeWidgetItem *item) {
        if (item->isExpanded()) {
            expandedItems.insert(item->text(0));
        }
        for (int i = 0; i < item->childCount(); ++i) {
            collectExpanded(item->child(i));
        }
    };
    for (int i = 0; i < m_sceneNodeTree->topLevelItemCount(); ++i) {
        collectExpanded(m_sceneNodeTree->topLevelItem(i));
    }
    
    m_sceneNodeTree->clear();
    
    // 获取场景中的所有节点
    QList<Node*> &nodes = m_scene->getNodes();
    
    for (Node *node : nodes) {
        QTreeWidgetItem *nodeItem = new QTreeWidgetItem();
        
        // 检查是否为组节点
        GroupNode *groupNode = dynamic_cast<GroupNode*>(node);
        
        if (groupNode) {
            // 组节点 - 使用特殊图标和样式，显示等级
            QString displayName = QString("📦 %1 [Lv.%2]").arg(node->getName()).arg(groupNode->getGroupLevel());
            nodeItem->setText(0, displayName);
            nodeItem->setToolTip(0, QString("组合节点: %1\n组件等级: %2\n包含 %3 个内部节点")
                                .arg(node->getName())
                                .arg(groupNode->getGroupLevel())
                                .arg(groupNode->getInternalNodes().size()));
            nodeItem->setForeground(0, QColor(100, 149, 237));  // 蓝色
            
            // 递归添加内部节点作为子项
            for (Node *internalNode : groupNode->getInternalNodes()) {
                addNodeToTree(nodeItem, internalNode, 1, expandedItems);
            }
            
            // 恢复展开状态
            if (expandedItems.contains(displayName)) {
                nodeItem->setExpanded(true);
            }
        } else {
            // 普通节点
            QString displayType = node->getDisplayTypeName();
            if (displayType.isEmpty()) {
                displayType = node->getType();
            }
            nodeItem->setText(0, QString("● %1").arg(node->getName()));
            nodeItem->setToolTip(0, QString("节点: %1\n类型: %2")
                                .arg(node->getName())
                                .arg(displayType));
            nodeItem->setForeground(0, QColor(81, 207, 102));  // 绿色
        }
        
        // 存储节点指针
        nodeItem->setData(0, Qt::UserRole, reinterpret_cast<quintptr>(node));
        nodeItem->setData(0, Qt::UserRole + 1, false);  // 标记为非内部节点
        
        m_sceneNodeTree->addTopLevelItem(nodeItem);
    }
    
    // 更新标题显示节点数量
    m_sceneNodeTree->setHeaderLabel(QString("绘图中的节点 (%1)").arg(nodes.size()));
}

/**
 * @brief 场景节点树项被点击时的处理
 * @param item 被点击的树项
 * @param column 列索引
 * 
 * 点击树项时，在场景中选中并定位到对应节点
 */
void MainWindow::onSceneNodeTreeItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    
    if (!item) return;
    
    // 获取存储的节点指针
    quintptr ptr = item->data(0, Qt::UserRole).toULongLong();
    bool isInternalNode = item->data(0, Qt::UserRole + 1).toBool();
    
    if (ptr == 0) return;
    
    Node *node = reinterpret_cast<Node*>(ptr);
    
    // 如果是内部节点，提示用户
    if (isInternalNode) {
        statusBar()->showMessage(QString("内部节点 '%1' (属于组节点内部，不可直接选中)")
                                .arg(node->getName()));
        return;
    }
    
    // 清除当前选择并选中目标节点
    m_scene->clearSelection();
    node->setSelected(true);
    
    // 将视图中心移动到节点位置
    m_view->centerOn(node);
    
    // 发出选择变化信号
    emit m_scene->selectionChanged(node);
    
    statusBar()->showMessage(QString("已定位到节点: %1").arg(node->getName()));
}


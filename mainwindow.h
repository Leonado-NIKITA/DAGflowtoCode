/**
 * @file mainwindow.h
 * @brief 主窗口类头文件，定义应用程序的主界面
 * @author 
 * @version 1.0.0
 * @date 2024
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>       // Qt主窗口基类
#include <QMap>              // Qt映射容器
#include <QGraphicsScene>    // Qt图形场景

// 前向声明
class NodeScene;            // 节点场景类
class NodeView;             // 节点视图类
class QGraphicsScene;       // 图形场景类
class QTreeWidget;          // 树形控件类
class DraggableNodeTree;    // 可拖拽节点树控件类
class QTreeWidgetItem;      // 树形项类
class QTextEdit;            // 文本编辑控件类
class QLineEdit;            // 单行输入框类
class QComboBox;            // 下拉框类
class QPushButton;          // 按钮类

/**
 * @class MainWindow
 * @brief 主窗口类，负责管理整个应用程序的用户界面
 * 
 * 该类继承自QMainWindow，包含节点编辑器的所有UI组件：
 * - 菜单栏和工具栏
 * - 节点库停靠窗口
 * - 属性编辑器停靠窗口
 * - 代码输出停靠窗口
 * - 中心绘图区域
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针，默认为nullptr
     */
    MainWindow(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~MainWindow();

private slots:
    /**
     * @brief 节点被选中时的槽函数
     * @param item 被选中的图形项指针
     */
    void onNodeSelected(QGraphicsItem* item);
    
    /**
     * @brief 连接创建完成时的槽函数
     */
    void onConnectionCreated();
    
    /**
     * @brief 生成代码槽函数
     */
    void onGenerateCode();
    
    /**
     * @brief 保存项目槽函数
     */
    void onSaveProject();
    
    /**
     * @brief 加载项目槽函数
     */
    void onLoadProject();
    
    /**
     * @brief 清空画布槽函数
     */
    void onClearCanvas();
    
    /**
     * @brief 添加节点槽函数
     */
    void onAddNode();
    
    /**
     * @brief 更新节点属性槽函数
     */
    void onUpdateNodeProperties();
    
    /**
     * @brief 删除选中项槽函数
     */
    void onDeleteSelected();
    
    /**
     * @brief 创建自定义节点槽函数
     */
    void onCreateCustomNode();
    
    /**
     * @brief 编辑选中的节点模板槽函数
     */
    void onEditNodeTemplate();
    
    /**
     * @brief 删除选中的自定义节点模板槽函数
     */
    void onDeleteNodeTemplate();
    
    /**
     * @brief 保存节点库槽函数
     */
    void onSaveNodeLibrary();
    
    /**
     * @brief 加载节点库槽函数
     */
    void onLoadNodeLibrary();
    
    /**
     * @brief 刷新节点库显示槽函数
     */
    void refreshNodeLibrary();
    
    /**
     * @brief 更新连线属性槽函数
     */
    void onUpdateConnectionProperties();
    
    /**
     * @brief 连线类型改变槽函数
     * @param index 新的类型索引
     */
    void onConnectionLineTypeChanged(int index);
    
    /**
     * @brief 导出生成代码为JSON格式槽函数
     */
    void onExportCodeAsJson();
    
    /**
     * @brief 导出生成代码为Python格式槽函数
     */
    void onExportCodeAsPython();
    
    /**
     * @brief 导出生成代码为YAML配置格式槽函数
     */
    void onExportCodeAsYaml();
    
    /**
     * @brief 更新场景节点树显示槽函数
     */
    void updateSceneNodeTree();
    
    /**
     * @brief 场景节点树项被点击时的槽函数
     * @param item 被点击的树项
     * @param column 列索引
     */
    void onSceneNodeTreeItemClicked(QTreeWidgetItem *item, int column);

private:
    /**
     * @brief 设置用户界面
     */
    void setupUI();
    
    /**
     * @brief 创建菜单栏
     */
    void createMenus();
    
    /**
     * @brief 创建工具栏
     */
    void createToolBars();
    
    /**
     * @brief 创建停靠窗口
     */
    void createDockWidgets();
    
    /**
     * @brief 建立信号槽连接
     */
    void setupConnections();
    
    // 核心组件
    NodeScene *m_scene;        // 节点场景，管理所有节点和连接
    NodeView *m_view;          // 节点视图，显示场景内容
    
    // UI界面组件
    DraggableNodeTree *m_nodeLibrary;  // 节点库树形控件（支持拖拽）
    QTextEdit *m_codeOutput;           // 代码输出文本框
    QTextEdit *m_propertyEditor;       // 属性编辑器（未使用）
    QLineEdit *m_nodeNameEdit;         // 节点名称输入框
    QComboBox *m_nodeTypeCombo;        // 节点类型下拉框
    QLineEdit *m_nodeParamsEdit;       // 节点参数输入框
    QPushButton *m_updatePropsButton;  // 更新属性按钮
    QTreeWidget *m_connectionTree;     // 连接关系树形控件
    
    // 连线属性编辑组件
    QWidget *m_nodePropsWidget;        // 节点属性组件容器
    QWidget *m_connPropsWidget;        // 连线属性组件容器
    QLineEdit *m_connFromNodeEdit;     // 连线源节点显示
    QLineEdit *m_connToNodeEdit;       // 连线目标节点显示
    QLineEdit *m_connFromPortEdit;     // 连线源端口显示
    QLineEdit *m_connToPortEdit;       // 连线目标端口显示
    QComboBox *m_connLineTypeCombo;    // 连线类型下拉框
    
    // 当前选中的连线
    class Connection *m_selectedConnection;  // 当前选中的连线指针
    
    // 节点模板映射表
    QMap<QString, QString> m_nodeTemplates;  // 节点类型到显示名称的映射
    
    // 场景节点树浏览
    QTreeWidget *m_sceneNodeTree;  // 场景节点树控件
    
    // 组节点属性编辑
    QWidget *m_groupPropsWidget;   // 组节点属性组件容器
    class QSpinBox *m_groupLevelSpinBox;  // 组件等级输入框
};

#endif // MAINWINDOW_H

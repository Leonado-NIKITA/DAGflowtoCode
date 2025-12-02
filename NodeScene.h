/**
 * @file NodeScene.h
 * @brief 节点场景类头文件，管理节点编辑器的所有图形元素
 * @author 
 * @version 1.0.0
 * @date 2024
 */

#ifndef NODESCENE_H
#define NODESCENE_H

#include "qjsonobject.h"
#include <QGraphicsScene>              // Qt图形场景基类
#include <QObject>                     // Qt对象基类
#include <QDebug>                      // 调试输出类
#include <QUndoStack>                  // 撤销栈类

// 调试开关：设置为true启用连线调试输出
static const bool DEBUG_CONNECTION = true;

// 前向声明
class Node;                            // 节点类
class Connection;                      // 连接线类
class GroupNode;                       // 组节点类

/**
 * @class NodeScene
 * @brief 节点场景类，继承自QGraphicsScene
 * 
 * 该类负责管理节点编辑器中的所有图形元素：
 * - 节点的创建、删除和管理
 * - 连接线的创建和删除
 * - 场景的交互模式管理
 * - 数据的序列化和反序列化
 * - 流程图的验证
 */
class NodeScene : public QGraphicsScene
{
    Q_OBJECT

public:
    /**
     * @brief 连接状态枚举
     */
    enum ConnectionState {
        None,            // 无连接操作
        FromNodeClicked  // 已点击源节点，等待目标节点
    };
    
    /**
     * @brief 构造函数
     * @param parent 父对象指针，默认为nullptr
     */
    NodeScene(QObject *parent = nullptr);
    
    // 连接状态管理
    ConnectionState getConnectionState() const { return m_connectionState; }  ///< 获取当前连接状态
    
    /**
     * @brief 获取撤销栈
     * @return 撤销栈指针
     */
    QUndoStack* undoStack() { return &m_undoStack; }
    
    /**
     * @brief 在指定位置添加新节点
     * @param type 节点类型
     * @param position 节点位置
     */
    void addNode(const QString &type, const QPointF &position);
    
    /**
     * @brief 创建节点（不添加撤销命令，供撤销系统使用）
     * @param type 节点类型
     * @param position 节点位置
     * @return 创建的节点指针
     */
    Node* createNode(const QString &type, const QPointF &position);
    
    /**
     * @brief 从场景移除节点（不删除，供撤销系统使用）
     * @param node 节点指针
     */
    void removeNodeFromScene(Node *node);
    
    /**
     * @brief 恢复节点到场景（供撤销系统使用）
     * @param node 节点指针
     */
    void restoreNodeToScene(Node *node);
    
    /**
     * @brief 创建连接（不添加撤销命令，供撤销系统使用）
     */
    Connection* createConnection(Node *fromNode, int fromPort, Node *toNode, int toPort);
    
    /**
     * @brief 从场景移除连接（不删除，供撤销系统使用）
     */
    void removeConnectionFromScene(Connection *conn);
    
    /**
     * @brief 恢复连接到场景（供撤销系统使用）
     */
    void restoreConnectionToScene(Connection *conn);
    
    /**
     * @brief 获取节点列表（供撤销系统使用）
     * @return 节点列表的引用
     */
    QList<Node*>& getNodes() { return m_nodes; }
    
    /**
     * @brief 获取连接列表（供撤销系统使用）
     * @return 连接列表的引用
     */
    QList<Connection*>& getConnections() { return m_connections; }
    
    /**
     * @brief 在两个节点之间创建连接（兼容旧版本，使用第一个端口）
     * @param fromNode 源节点
     * @param toNode 目标节点
     */
    void addConnection(Node *fromNode, Node *toNode);
    
    /**
     * @brief 在两个节点的指定端口之间创建连接
     * @param fromNode 源节点
     * @param fromPortIndex 源节点的输出端口索引
     * @param toNode 目标节点
     * @param toPortIndex 目标节点的输入端口索引
     */
    void addConnection(Node *fromNode, int fromPortIndex, Node *toNode, int toPortIndex);
    
    /**
     * @brief 验证当前流程图的有效性
     * @return 验证是否通过
     */
    bool validateFlow() const;
    
    /**
     * @brief 获取流程图的JSON数据
     * @return 包含所有节点和连接信息的JSON对象
     */
    QJsonObject getFlowData() const;
    
    /**
     * @brief 从JSON数据加载流程图
     * @param data 包含流程图信息的JSON对象
     */
    void loadFlowData(const QJsonObject &data);
    
    /**
     * @brief 删除当前选中的所有元素
     */
    void deleteSelected();
    
    /**
     * @brief 获取当前选中的节点
     * @return 选中的节点指针，如果没有则返回nullptr
     */
    QGraphicsItem* getSelectedNode() const;
    
    /**
     * @brief 清除所有端口高亮
     */
    void clearPortHighlights();
    
    /**
     * @brief 更新端口高亮状态
     * @param mousePos 鼠标当前位置
     */
    void updatePortHighlights(const QPointF &mousePos);
    
    /**
     * @brief 取消当前连线操作
     */
    void cancelConnection();
    
    /**
     * @brief 清理临时连线状态
     */
    void cleanupTempConnection();
    
    /**
     * @brief 复制选中的元素到剪贴板
     */
    void copySelected();
    
    /**
     * @brief 粘贴剪贴板中的元素
     * @param offset 粘贴位置偏移量
     */
    void paste(const QPointF &offset = QPointF(30, 30));
    
    /**
     * @brief 剪切选中的元素
     */
    void cutSelected();
    
    /**
     * @brief 全选所有元素
     */
    void selectAll();
    
    /**
     * @brief 检查是否有可粘贴的内容
     * @return 如果有可粘贴内容返回true
     */
    bool canPaste() const;
    
    /**
     * @brief 将选中的节点打包为组节点
     * @return 如果成功打包返回true
     */
    bool groupSelected();
    
    /**
     * @brief 拆分选中的组节点
     * @return 如果成功拆分返回true
     */
    bool ungroupSelected();
    
    /**
     * @brief 检查是否可以打包选中的节点
     * @return 如果可以打包返回true
     */
    bool canGroup() const;
    
    /**
     * @brief 检查是否可以拆分选中的组节点
     * @return 如果可以拆分返回true
     */
    bool canUngroup() const;
    
signals:
    /**
     * @brief 选择发生变化时发出的信号
     * @param item 被选中的图形项，如果无选中则为nullptr
     */
    void selectionChanged(QGraphicsItem* item);
    
    /**
     * @brief 连接创建完成时发出的信号
     */
    void connectionCreated();

public slots:
    /**
     * @brief 当节点模板更新时更新场景中的相应节点
     * @param typeId 更新的模板类型ID
     */
    void onTemplateUpdated(const QString &typeId);

protected:
    /**
     * @brief 鼠标按下事件处理
     * @param event 鼠标事件对象
     */
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    
    /**
     * @brief 鼠标移动事件处理
     * @param event 鼠标事件对象
     */
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    
    /**
     * @brief 鼠标释放事件处理
     * @param event 鼠标事件对象
     */
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    ConnectionState m_connectionState; ///< 当前连接状态
    Node *m_tempFromNode;           ///< 临时源节点（用于连接创建）
    int m_tempFromPortIndex;        ///< 临时源端口索引
    QGraphicsLineItem *m_tempLine;  ///< 临时连接线（用于可视化连接过程）
    
    QList<Node*> m_nodes;            ///< 场景中所有节点的列表
    QList<Connection*> m_connections; ///< 场景中所有连接线的列表
    
    QJsonObject m_clipboard;         ///< 剪贴板数据（存储复制的节点和连接）
    QUndoStack m_undoStack;          ///< 撤销/重做栈
};

#endif // NODESCENE_H

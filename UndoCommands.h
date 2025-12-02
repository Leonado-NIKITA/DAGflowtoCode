/**
 * @file UndoCommands.h
 * @brief 撤销/重做命令类头文件
 * @author
 * @version 1.0.0
 * @date 2024
 */

#ifndef UNDOCOMMANDS_H
#define UNDOCOMMANDS_H

#include <QUndoCommand>
#include <QPointF>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include "GroupNode.h"

class NodeScene;
class Node;
class Connection;

/**
 * @class AddNodeCommand
 * @brief 添加节点的撤销命令
 */
class AddNodeCommand : public QUndoCommand
{
public:
    AddNodeCommand(NodeScene *scene, const QString &type, const QPointF &pos, 
                   QUndoCommand *parent = nullptr);
    ~AddNodeCommand();
    
    void undo() override;
    void redo() override;
    
    Node* getNode() const { return m_node; }

private:
    NodeScene *m_scene;
    QString m_type;
    QPointF m_pos;
    Node *m_node;
    bool m_ownsNode;  // 是否拥有节点的所有权
};

/**
 * @class DeleteCommand
 * @brief 删除节点和连接的撤销命令
 */
class DeleteCommand : public QUndoCommand
{
public:
    DeleteCommand(NodeScene *scene, const QList<Node*> &nodes, 
                  const QList<Connection*> &connections, QUndoCommand *parent = nullptr);
    ~DeleteCommand();
    
    void undo() override;
    void redo() override;

private:
    NodeScene *m_scene;
    QList<QJsonObject> m_nodeData;      // 节点的JSON数据
    QList<QJsonObject> m_connectionData; // 连接的JSON数据
    QList<Node*> m_deletedNodes;
    QList<Connection*> m_deletedConnections;
    bool m_ownsItems;  // 是否拥有项目的所有权
};

/**
 * @class MoveNodeCommand
 * @brief 移动节点的撤销命令
 */
class MoveNodeCommand : public QUndoCommand
{
public:
    MoveNodeCommand(Node *node, const QPointF &oldPos, const QPointF &newPos,
                    QUndoCommand *parent = nullptr);
    
    void undo() override;
    void redo() override;
    
    int id() const override { return 1; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    Node *m_node;
    QPointF m_oldPos;
    QPointF m_newPos;
};

/**
 * @class MoveNodesCommand
 * @brief 移动多个节点的撤销命令
 */
class MoveNodesCommand : public QUndoCommand
{
public:
    MoveNodesCommand(const QList<Node*> &nodes, const QList<QPointF> &oldPositions,
                     const QList<QPointF> &newPositions, QUndoCommand *parent = nullptr);
    
    void undo() override;
    void redo() override;

private:
    QList<Node*> m_nodes;
    QList<QPointF> m_oldPositions;
    QList<QPointF> m_newPositions;
};

/**
 * @class AddConnectionCommand
 * @brief 添加连接的撤销命令
 */
class AddConnectionCommand : public QUndoCommand
{
public:
    AddConnectionCommand(NodeScene *scene, Node *fromNode, int fromPort,
                         Node *toNode, int toPort, QUndoCommand *parent = nullptr);
    ~AddConnectionCommand();
    
    void undo() override;
    void redo() override;
    
    Connection* getConnection() const { return m_connection; }

private:
    NodeScene *m_scene;
    Node *m_fromNode;
    Node *m_toNode;
    int m_fromPort;
    int m_toPort;
    Connection *m_connection;
    bool m_ownsConnection;
};

/**
 * @class PasteCommand
 * @brief 粘贴操作的撤销命令
 */
class PasteCommand : public QUndoCommand
{
public:
    PasteCommand(NodeScene *scene, const QJsonObject &clipboardData, 
                 const QPointF &offset, QUndoCommand *parent = nullptr);
    ~PasteCommand();
    
    void undo() override;
    void redo() override;

private:
    NodeScene *m_scene;
    QJsonObject m_clipboardData;
    QPointF m_offset;
    QList<Node*> m_pastedNodes;
    QList<Connection*> m_pastedConnections;
    bool m_ownsItems;
};

/**
 * @class GroupNodesCommand
 * @brief 打包节点的撤销命令
 */
class GroupNodesCommand : public QUndoCommand
{
public:
    GroupNodesCommand(NodeScene *scene, const QList<Node*> &nodes,
                      const QList<Connection*> &internalConnections,
                      const QList<ExternalConnection> &externalConnections,
                      QUndoCommand *parent = nullptr);
    ~GroupNodesCommand();
    
    void undo() override;
    void redo() override;
    
    GroupNode* getGroupNode() const { return m_groupNode; }

private:
    NodeScene *m_scene;
    QList<Node*> m_nodes;                           ///< 被打包的节点
    QList<Connection*> m_internalConnections;       ///< 内部连接
    QList<ExternalConnection> m_externalConnections;///< 外部连接信息
    QList<Connection*> m_externalConnectionPtrs;    ///< 外部连接指针
    QMap<Node*, QPointF> m_originalPositions;       ///< 节点原始位置
    GroupNode *m_groupNode;                         ///< 创建的组节点
    QList<Connection*> m_newExternalConnections;    ///< 与组节点的新连接
    bool m_firstRedo;
};

/**
 * @class UngroupNodesCommand
 * @brief 拆分组节点的撤销命令
 */
class UngroupNodesCommand : public QUndoCommand
{
public:
    UngroupNodesCommand(NodeScene *scene, GroupNode *groupNode,
                        QUndoCommand *parent = nullptr);
    ~UngroupNodesCommand();
    
    void undo() override;
    void redo() override;

private:
    NodeScene *m_scene;
    GroupNode *m_groupNode;
    QList<Node*> m_internalNodes;
    QList<Connection*> m_internalConnections;
    QList<ExternalConnection> m_externalConnections;
    QList<Connection*> m_groupExternalConnections;  ///< 组节点的外部连接
    QMap<Node*, QPointF> m_originalPositions;
    bool m_firstRedo;
};

#endif // UNDOCOMMANDS_H


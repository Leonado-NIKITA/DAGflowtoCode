/**
 * @file UndoCommands.cpp
 * @brief 撤销/重做命令类实现文件
 * @author
 * @version 1.0.0
 * @date 2024
 */

#include "UndoCommands.h"
#include "NodeScene.h"
#include "Node.h"
#include "Connection.h"
#include "GroupNode.h"
#include <QJsonArray>

// ==================== AddNodeCommand ====================

AddNodeCommand::AddNodeCommand(NodeScene *scene, const QString &type, const QPointF &pos,
                               QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_type(type)
    , m_pos(pos)
    , m_node(nullptr)
    , m_ownsNode(false)
{
    setText(QString("添加节点"));
}

AddNodeCommand::~AddNodeCommand()
{
    if (m_ownsNode && m_node) {
        delete m_node;
    }
}

void AddNodeCommand::undo()
{
    if (m_node) {
        m_scene->removeNodeFromScene(m_node);
        m_ownsNode = true;  // 撤销后我们拥有节点
    }
}

void AddNodeCommand::redo()
{
    if (!m_node) {
        // 首次执行，创建节点
        m_node = m_scene->createNode(m_type, m_pos);
    } else {
        // 重做，恢复节点到场景
        m_scene->restoreNodeToScene(m_node);
    }
    m_ownsNode = false;  // 场景拥有节点
}

// ==================== DeleteCommand ====================

DeleteCommand::DeleteCommand(NodeScene *scene, const QList<Node*> &nodes,
                             const QList<Connection*> &connections, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_deletedNodes(nodes)
    , m_deletedConnections(connections)
    , m_ownsItems(false)
{
    setText(QString("删除 %1 个项目").arg(nodes.size() + connections.size()));
    
    // 保存节点数据
    for (Node *node : nodes) {
        m_nodeData.append(node->toJson());
    }
    
    // 保存连接数据
    for (Connection *conn : connections) {
        QJsonObject connData;
        connData["fromNodePtr"] = QString::number(reinterpret_cast<quintptr>(conn->getFromNode()));
        connData["toNodePtr"] = QString::number(reinterpret_cast<quintptr>(conn->getToNode()));
        connData["fromPort"] = conn->getFromPortIndex();
        connData["toPort"] = conn->getToPortIndex();
        connData["lineType"] = static_cast<int>(conn->getLineType());
        m_connectionData.append(connData);
    }
}

DeleteCommand::~DeleteCommand()
{
    if (m_ownsItems) {
        for (Connection *conn : m_deletedConnections) {
            delete conn;
        }
        for (Node *node : m_deletedNodes) {
            delete node;
        }
    }
}

void DeleteCommand::undo()
{
    // 恢复节点
    for (Node *node : m_deletedNodes) {
        m_scene->restoreNodeToScene(node);
    }
    
    // 恢复连接
    for (Connection *conn : m_deletedConnections) {
        m_scene->restoreConnectionToScene(conn);
    }
    
    m_ownsItems = false;  // 场景拥有项目
}

void DeleteCommand::redo()
{
    // 移除连接
    for (Connection *conn : m_deletedConnections) {
        m_scene->removeConnectionFromScene(conn);
    }
    
    // 移除节点
    for (Node *node : m_deletedNodes) {
        m_scene->removeNodeFromScene(node);
    }
    
    m_ownsItems = true;  // 我们拥有项目
}

// ==================== MoveNodeCommand ====================

MoveNodeCommand::MoveNodeCommand(Node *node, const QPointF &oldPos, const QPointF &newPos,
                                 QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_node(node)
    , m_oldPos(oldPos)
    , m_newPos(newPos)
{
    setText(QString("移动节点"));
}

void MoveNodeCommand::undo()
{
    m_node->setPos(m_oldPos);
    // 更新所有连接线
    for (Connection *conn : m_node->getConnections()) {
        conn->updatePath();
    }
}

void MoveNodeCommand::redo()
{
    m_node->setPos(m_newPos);
    // 更新所有连接线
    for (Connection *conn : m_node->getConnections()) {
        conn->updatePath();
    }
}

bool MoveNodeCommand::mergeWith(const QUndoCommand *other)
{
    if (other->id() != id()) {
        return false;
    }
    
    const MoveNodeCommand *moveCmd = static_cast<const MoveNodeCommand*>(other);
    if (moveCmd->m_node != m_node) {
        return false;
    }
    
    m_newPos = moveCmd->m_newPos;
    return true;
}

// ==================== MoveNodesCommand ====================

MoveNodesCommand::MoveNodesCommand(const QList<Node*> &nodes, const QList<QPointF> &oldPositions,
                                   const QList<QPointF> &newPositions, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_nodes(nodes)
    , m_oldPositions(oldPositions)
    , m_newPositions(newPositions)
{
    setText(QString("移动 %1 个节点").arg(nodes.size()));
}

void MoveNodesCommand::undo()
{
    for (int i = 0; i < m_nodes.size(); ++i) {
        m_nodes[i]->setPos(m_oldPositions[i]);
        // 更新所有连接线
        for (Connection *conn : m_nodes[i]->getConnections()) {
            conn->updatePath();
        }
    }
}

void MoveNodesCommand::redo()
{
    for (int i = 0; i < m_nodes.size(); ++i) {
        m_nodes[i]->setPos(m_newPositions[i]);
        // 更新所有连接线
        for (Connection *conn : m_nodes[i]->getConnections()) {
            conn->updatePath();
        }
    }
}

// ==================== AddConnectionCommand ====================

AddConnectionCommand::AddConnectionCommand(NodeScene *scene, Node *fromNode, int fromPort,
                                           Node *toNode, int toPort, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_fromNode(fromNode)
    , m_toNode(toNode)
    , m_fromPort(fromPort)
    , m_toPort(toPort)
    , m_connection(nullptr)
    , m_ownsConnection(false)
{
    setText(QString("添加连接"));
}

AddConnectionCommand::~AddConnectionCommand()
{
    if (m_ownsConnection && m_connection) {
        delete m_connection;
    }
}

void AddConnectionCommand::undo()
{
    if (m_connection) {
        m_scene->removeConnectionFromScene(m_connection);
        m_ownsConnection = true;
    }
}

void AddConnectionCommand::redo()
{
    if (!m_connection) {
        // 首次执行，创建连接
        m_connection = m_scene->createConnection(m_fromNode, m_fromPort, m_toNode, m_toPort);
    } else {
        // 重做，恢复连接
        m_scene->restoreConnectionToScene(m_connection);
    }
    m_ownsConnection = false;
}

// ==================== PasteCommand ====================

PasteCommand::PasteCommand(NodeScene *scene, const QJsonObject &clipboardData,
                           const QPointF &offset, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_clipboardData(clipboardData)
    , m_offset(offset)
    , m_ownsItems(false)
{
    QJsonArray nodesArray = clipboardData["nodes"].toArray();
    setText(QString("粘贴 %1 个项目").arg(nodesArray.size()));
}

PasteCommand::~PasteCommand()
{
    if (m_ownsItems) {
        for (Connection *conn : m_pastedConnections) {
            delete conn;
        }
        for (Node *node : m_pastedNodes) {
            delete node;
        }
    }
}

void PasteCommand::undo()
{
    // 先移除连接
    for (Connection *conn : m_pastedConnections) {
        m_scene->removeConnectionFromScene(conn);
    }
    
    // 再移除节点
    for (Node *node : m_pastedNodes) {
        m_scene->removeNodeFromScene(node);
    }
    
    m_ownsItems = true;
}

void PasteCommand::redo()
{
    if (m_pastedNodes.isEmpty()) {
        // 首次执行，创建节点和连接
        static int pasteCount = 1;
        
        // 用于映射复制ID到新节点
        QMap<QString, Node*> newNodeMap;
        
        // 创建节点
        QJsonArray nodesArray = m_clipboardData["nodes"].toArray();
        for (const QJsonValue &nodeValue : nodesArray) {
            QJsonObject nodeObj = nodeValue.toObject();
            QString copyId = nodeObj["copyId"].toString();
            
            // 创建新节点
            Node *newNode = Node::fromJson(nodeObj);
            
            // 设置新位置（相对位置 + 偏移）
            qreal newX = nodeObj["relX"].toDouble() + m_offset.x();
            qreal newY = nodeObj["relY"].toDouble() + m_offset.y();
            newNode->setPos(newX, newY);
            
            // 生成新名称
            newNode->setName(newNode->getName() + QString(" (副本%1)").arg(pasteCount));
            
            m_scene->addItem(newNode);
            m_scene->getNodes().append(newNode);
            newNodeMap[copyId] = newNode;
            m_pastedNodes.append(newNode);
            
            // 选中新创建的节点
            newNode->setSelected(true);
        }
        
        pasteCount++;
        
        // 创建连接
        QJsonArray connectionsArray = m_clipboardData["connections"].toArray();
        for (const QJsonValue &connValue : connectionsArray) {
            QJsonObject connObj = connValue.toObject();
            
            QString fromCopyId = connObj["fromCopyId"].toString();
            QString toCopyId = connObj["toCopyId"].toString();
            
            Node *fromNode = newNodeMap.value(fromCopyId);
            Node *toNode = newNodeMap.value(toCopyId);
            
            if (fromNode && toNode) {
                int fromPort = connObj["fromPort"].toInt();
                int toPort = connObj["toPort"].toInt();
                
                Connection *newConn = new Connection(fromNode, fromPort, toNode, toPort);
                
                // 恢复连线类型
                if (connObj.contains("lineType")) {
                    newConn->setLineType(static_cast<Connection::LineType>(connObj["lineType"].toInt()));
                }
                
                m_scene->addItem(newConn);
                m_scene->getConnections().append(newConn);
                m_pastedConnections.append(newConn);
            }
        }
    } else {
        // 重做，恢复节点和连接
        for (Node *node : m_pastedNodes) {
            m_scene->restoreNodeToScene(node);
            node->setSelected(true);
        }
        
        for (Connection *conn : m_pastedConnections) {
            m_scene->restoreConnectionToScene(conn);
        }
    }
    
    m_ownsItems = false;
}

// ==================== GroupNodesCommand ====================

GroupNodesCommand::GroupNodesCommand(NodeScene *scene, const QList<Node*> &nodes,
                                     const QList<Connection*> &internalConnections,
                                     const QList<ExternalConnection> &externalConnections,
                                     QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_nodes(nodes)
    , m_internalConnections(internalConnections)
    , m_externalConnections(externalConnections)
    , m_groupNode(nullptr)
    , m_firstRedo(true)
{
    setText(QString("打包 %1 个节点").arg(nodes.size()));
    
    // 保存原始位置
    for (Node *node : nodes) {
        m_originalPositions[node] = node->pos();
    }
    
    // 保存外部连接指针
    for (const ExternalConnection &ext : externalConnections) {
        m_externalConnectionPtrs.append(ext.originalConnection);
    }
}

GroupNodesCommand::~GroupNodesCommand()
{
    // 注意：节点和连接的生命周期由场景管理
}

void GroupNodesCommand::undo()
{
    if (!m_groupNode) return;
    
    // 移除组节点与外部的连接
    for (Connection *conn : m_newExternalConnections) {
        m_scene->removeConnectionFromScene(conn);
    }
    
    // 移除组节点
    m_scene->removeNodeFromScene(m_groupNode);
    
    // 恢复原始节点
    for (Node *node : m_nodes) {
        node->setPos(m_originalPositions[node]);
        m_scene->restoreNodeToScene(node);
    }
    
    // 恢复内部连接
    for (Connection *conn : m_internalConnections) {
        m_scene->restoreConnectionToScene(conn);
    }
    
    // 恢复外部连接
    for (Connection *conn : m_externalConnectionPtrs) {
        m_scene->restoreConnectionToScene(conn);
    }
}

void GroupNodesCommand::redo()
{
    if (m_firstRedo) {
        m_firstRedo = false;
        
        // 计算组节点位置（所有节点的中心）
        QPointF center(0, 0);
        for (Node *node : m_nodes) {
            center += node->pos();
        }
        center /= m_nodes.size();
        
        // 创建组节点
        m_groupNode = new GroupNode("组合", center);
        m_groupNode->setInternalNodes(m_nodes);
        m_groupNode->setInternalConnections(m_internalConnections);
        m_groupNode->setExternalConnections(m_externalConnections);
        m_groupNode->setOriginalPositions(m_originalPositions);
        m_groupNode->calculatePortMappings();
        
        // 移除外部连接
        for (Connection *conn : m_externalConnectionPtrs) {
            m_scene->removeConnectionFromScene(conn);
        }
        
        // 移除内部连接
        for (Connection *conn : m_internalConnections) {
            m_scene->removeConnectionFromScene(conn);
        }
        
        // 移除原始节点
        for (Node *node : m_nodes) {
            m_scene->removeNodeFromScene(node);
        }
        
        // 添加组节点
        m_scene->addItem(m_groupNode);
        m_scene->getNodes().append(m_groupNode);
        
        // 创建组节点与外部的新连接
        QList<PortMapping> inputMappings = m_groupNode->getInputPortMappings();
        QList<PortMapping> outputMappings = m_groupNode->getOutputPortMappings();
        
        for (int i = 0; i < m_externalConnections.size(); ++i) {
            const ExternalConnection &ext = m_externalConnections[i];
            Connection *newConn = nullptr;
            
            if (ext.isInput) {
                // 外部节点输出 -> 组节点输入
                // 找到对应的输入端口索引
                int groupPortIndex = 0;
                for (int j = 0; j < inputMappings.size(); ++j) {
                    if (inputMappings[j].internalNode == ext.internalNode &&
                        inputMappings[j].internalPortIndex == ext.internalPortIndex) {
                        groupPortIndex = j;
                        break;
                    }
                }
                newConn = new Connection(ext.externalNode, ext.externalPortIndex,
                                         m_groupNode, groupPortIndex);
            } else {
                // 组节点输出 -> 外部节点输入
                int groupPortIndex = 0;
                for (int j = 0; j < outputMappings.size(); ++j) {
                    if (outputMappings[j].internalNode == ext.internalNode &&
                        outputMappings[j].internalPortIndex == ext.internalPortIndex) {
                        groupPortIndex = j;
                        break;
                    }
                }
                newConn = new Connection(m_groupNode, groupPortIndex,
                                         ext.externalNode, ext.externalPortIndex);
            }
            
            if (newConn) {
                m_scene->addItem(newConn);
                m_scene->getConnections().append(newConn);
                m_newExternalConnections.append(newConn);
            }
        }
    } else {
        // 重做
        // 移除外部连接
        for (Connection *conn : m_externalConnectionPtrs) {
            m_scene->removeConnectionFromScene(conn);
        }
        
        // 移除内部连接
        for (Connection *conn : m_internalConnections) {
            m_scene->removeConnectionFromScene(conn);
        }
        
        // 移除原始节点
        for (Node *node : m_nodes) {
            m_scene->removeNodeFromScene(node);
        }
        
        // 恢复组节点
        m_scene->restoreNodeToScene(m_groupNode);
        
        // 恢复与组节点的连接
        for (Connection *conn : m_newExternalConnections) {
            m_scene->restoreConnectionToScene(conn);
        }
    }
}

// ==================== UngroupNodesCommand ====================

UngroupNodesCommand::UngroupNodesCommand(NodeScene *scene, GroupNode *groupNode,
                                         QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_groupNode(groupNode)
    , m_firstRedo(true)
{
    setText(QString("拆分组节点"));
    
    m_internalNodes = groupNode->getInternalNodes();
    m_internalConnections = groupNode->getInternalConnections();
    m_externalConnections = groupNode->getExternalConnections();
    m_originalPositions = groupNode->getOriginalPositions();
    
    // 获取组节点当前的外部连接
    m_groupExternalConnections = groupNode->getConnections();
}

UngroupNodesCommand::~UngroupNodesCommand()
{
    // 注意：节点和连接的生命周期由场景管理
}

void UngroupNodesCommand::undo()
{
    // 移除恢复的外部连接
    for (const ExternalConnection &ext : m_externalConnections) {
        if (ext.originalConnection) {
            m_scene->removeConnectionFromScene(ext.originalConnection);
        }
    }
    
    // 移除内部连接
    for (Connection *conn : m_internalConnections) {
        m_scene->removeConnectionFromScene(conn);
    }
    
    // 移除内部节点
    for (Node *node : m_internalNodes) {
        m_scene->removeNodeFromScene(node);
    }
    
    // 恢复组节点
    m_scene->restoreNodeToScene(m_groupNode);
    
    // 恢复组节点的外部连接
    for (Connection *conn : m_groupExternalConnections) {
        m_scene->restoreConnectionToScene(conn);
    }
}

void UngroupNodesCommand::redo()
{
    // 移除组节点的外部连接
    for (Connection *conn : m_groupExternalConnections) {
        m_scene->removeConnectionFromScene(conn);
    }
    
    // 移除组节点
    m_scene->removeNodeFromScene(m_groupNode);
    
    // 计算位置偏移（组节点当前位置与原始中心的偏移）
    QPointF groupPos = m_groupNode->pos();
    QPointF originalCenter(0, 0);
    for (auto it = m_originalPositions.begin(); it != m_originalPositions.end(); ++it) {
        originalCenter += it.value();
    }
    originalCenter /= m_originalPositions.size();
    QPointF offset = groupPos - originalCenter;
    
    // 恢复内部节点
    for (Node *node : m_internalNodes) {
        QPointF newPos = m_originalPositions[node] + offset;
        node->setPos(newPos);
        m_scene->restoreNodeToScene(node);
    }
    
    // 恢复内部连接
    for (Connection *conn : m_internalConnections) {
        m_scene->restoreConnectionToScene(conn);
        conn->updatePath();
    }
    
    // 恢复外部连接
    for (const ExternalConnection &ext : m_externalConnections) {
        if (ext.originalConnection) {
            m_scene->restoreConnectionToScene(ext.originalConnection);
            ext.originalConnection->updatePath();
        }
    }
}


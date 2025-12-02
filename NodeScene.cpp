#include "NodeScene.h"
#include "Node.h"
#include "Connection.h"
#include "GroupNode.h"
#include "NodeLibrary.h"
#include "NodeTemplate.h"
#include "UndoCommands.h"
#include <QGraphicsSceneMouseEvent>
#include <QJsonArray>
#include <QJsonDocument>

NodeScene::NodeScene(QObject *parent)
    : QGraphicsScene(parent)
    , m_connectionState(None)
    , m_tempFromNode(nullptr)
    , m_tempFromPortIndex(0)
    , m_tempLine(nullptr)
{
    setSceneRect(-2000, -2000, 4000, 4000);
    
    // 连接节点库模板更新信号，当模板更新时同步更新场景中的节点
    connect(NodeLibrary::instance(), &NodeLibrary::templateUpdated,
            this, &NodeScene::onTemplateUpdated);
}

void NodeScene::addNode(const QString &type, const QPointF &position)
{
    AddNodeCommand *cmd = new AddNodeCommand(this, type, position);
    m_undoStack.push(cmd);
}

/**
 * @brief 创建节点（不添加撤销命令，供撤销系统使用）
 */
Node* NodeScene::createNode(const QString &type, const QPointF &position)
{
    static int nodeCount = 1;
    QString name = QString("节点%1").arg(nodeCount++);
    
    Node *node = new Node(type, name, position);
    addItem(node);
    m_nodes.append(node);
    
    return node;
}

/**
 * @brief 从场景移除节点（不删除，供撤销系统使用）
 */
void NodeScene::removeNodeFromScene(Node *node)
{
    m_nodes.removeAll(node);
    removeItem(node);
}

/**
 * @brief 恢复节点到场景（供撤销系统使用）
 */
void NodeScene::restoreNodeToScene(Node *node)
{
    addItem(node);
    m_nodes.append(node);
}

/**
 * @brief 创建连接（不添加撤销命令，供撤销系统使用）
 */
Connection* NodeScene::createConnection(Node *fromNode, int fromPort, Node *toNode, int toPort)
{
    Connection *connection = new Connection(fromNode, fromPort, toNode, toPort);
    addItem(connection);
    m_connections.append(connection);
    emit connectionCreated();
    return connection;
}

/**
 * @brief 从场景移除连接（不删除，供撤销系统使用）
 */
void NodeScene::removeConnectionFromScene(Connection *conn)
{
    if (conn->getFromNode()) {
        conn->getFromNode()->removeConnection(conn);
    }
    if (conn->getToNode()) {
        conn->getToNode()->removeConnection(conn);
    }
    m_connections.removeAll(conn);
    removeItem(conn);
}

/**
 * @brief 恢复连接到场景（供撤销系统使用）
 */
void NodeScene::restoreConnectionToScene(Connection *conn)
{
    if (conn->getFromNode()) {
        conn->getFromNode()->addConnection(conn);
    }
    if (conn->getToNode()) {
        conn->getToNode()->addConnection(conn);
    }
    addItem(conn);
    m_connections.append(conn);
}

void NodeScene::addConnection(Node *fromNode, Node *toNode)
{
    // 使用默认端口（第一个端口）
    addConnection(fromNode, 0, toNode, 0);
}

void NodeScene::addConnection(Node *fromNode, int fromPortIndex, Node *toNode, int toPortIndex)
{
    if (DEBUG_CONNECTION) {
        qDebug() << "=== addConnection ===";
        qDebug() << "源节点:" << (fromNode ? fromNode->getName() : "nullptr")
                 << "端口:" << fromPortIndex;
        qDebug() << "目标节点:" << (toNode ? toNode->getName() : "nullptr")
                 << "端口:" << toPortIndex;
    }
    
    AddConnectionCommand *cmd = new AddConnectionCommand(this, fromNode, fromPortIndex, toNode, toPortIndex);
    m_undoStack.push(cmd);
    
    if (DEBUG_CONNECTION) {
        qDebug() << "连接创建完成，当前连接总数:" << m_connections.size();
    }
}

bool NodeScene::validateFlow() const
{
    // 检查连接有效性
    for (Connection *conn : m_connections) {
        if (!m_nodes.contains(conn->getFromNode()) || !m_nodes.contains(conn->getToNode())) {
            return false;
        }
    }
    
    // 检查输入输出匹配（简化验证）
    // 实际应用中需要更复杂的业务逻辑验证
    
    return true;
}

QJsonObject NodeScene::getFlowData() const
{
    QJsonObject flowData;
    flowData["metadata"] = QJsonObject{
        {"title", "可视化节点编辑器流程图"},
        {"created", QDateTime::currentDateTime().toString(Qt::ISODate)},
        {"version", "1.2"}  // 版本升级以支持组节点
    };
    
    // 生成标准节点数组
    QJsonArray nodesArray;
    for (Node *node : m_nodes) {
        QJsonObject nodeObj;
        nodeObj["id"] = QString("node_%1").arg(reinterpret_cast<quintptr>(node));
        nodeObj["type"] = node->getType();
        nodeObj["name"] = node->getName();
        nodeObj["position"] = QJsonObject{
            {"x", node->x()},
            {"y", node->y()}
        };
        // 保存端口数量
        nodeObj["inputPortCount"] = node->getInputPortCount();
        nodeObj["outputPortCount"] = node->getOutputPortCount();
        // 保存自定义属性
        if (node->hasCustomColor()) {
            nodeObj["customColor"] = node->getCustomColor().name();
        }
        if (!node->getDisplayTypeName().isEmpty()) {
            nodeObj["displayTypeName"] = node->getDisplayTypeName();
        }
        nodeObj["parameters"] = QJsonObject();  // 暂时为空对象
        
        // 检查是否为组节点
        if (GroupNode *groupNode = dynamic_cast<GroupNode*>(node)) {
            nodeObj["isGroup"] = true;
            // 保存内部节点
            QJsonArray internalNodesArray;
            for (Node *internalNode : groupNode->getInternalNodes()) {
                internalNodesArray.append(internalNode->toJson());
            }
            nodeObj["internalNodes"] = internalNodesArray;
            
            // 保存内部连接
            QJsonArray internalConnsArray;
            for (Connection *conn : groupNode->getInternalConnections()) {
                internalConnsArray.append(conn->toJson());
            }
            nodeObj["internalConnections"] = internalConnsArray;
            
            // 保存原始位置
            QJsonArray origPosArray;
            QMap<Node*, QPointF> origPos = groupNode->getOriginalPositions();
            for (auto it = origPos.begin(); it != origPos.end(); ++it) {
                QJsonObject posObj;
                posObj["nodeName"] = it.key()->getName();
                posObj["x"] = it.value().x();
                posObj["y"] = it.value().y();
                origPosArray.append(posObj);
            }
            nodeObj["originalPositions"] = origPosArray;
        }
        
        nodesArray.append(nodeObj);
    }
    flowData["nodes"] = nodesArray;
    
    // 生成标准连接数组（包含端口信息和线型）
    QJsonArray connectionsArray;
    for (Connection *conn : m_connections) {
        QString fromNodeId = QString("node_%1").arg(reinterpret_cast<quintptr>(conn->getFromNode()));
        QString toNodeId = QString("node_%1").arg(reinterpret_cast<quintptr>(conn->getToNode()));
        
        QJsonObject connObj;
        connObj["from"] = fromNodeId;
        connObj["fromPort"] = conn->getFromPortIndex();
        connObj["to"] = toNodeId;
        connObj["toPort"] = conn->getToPortIndex();
        connObj["lineType"] = static_cast<int>(conn->getLineType());
        connectionsArray.append(connObj);
    }
    flowData["connections"] = connectionsArray;
    
    return flowData;
}

void NodeScene::loadFlowData(const QJsonObject &data)
{
    clear();
    m_nodes.clear();
    m_connections.clear();
    
    QJsonArray nodesArray = data["nodes"].toArray();
    QMap<QString, Node*> nodeMap;
    
    // 首先创建所有节点（包括组节点）
    for (const QJsonValue &nodeValue : nodesArray) {
        QJsonObject nodeObj = nodeValue.toObject();
        Node *node = nullptr;
        
        // 检查是否为组节点
        if (nodeObj["isGroup"].toBool(false)) {
            // 创建组节点
            QString name = nodeObj["name"].toString();
            QJsonObject posObj = nodeObj["position"].toObject();
            QPointF position(posObj["x"].toDouble(), posObj["y"].toDouble());
            
            GroupNode *groupNode = new GroupNode(name, position);
            
            // 恢复内部节点
            QJsonArray internalNodesArray = nodeObj["internalNodes"].toArray();
            QList<Node*> internalNodes;
            QMap<QString, Node*> internalNodeMap;
            
            for (const QJsonValue &internalValue : internalNodesArray) {
                QJsonObject internalObj = internalValue.toObject();
                Node *internalNode = Node::fromJson(internalObj);
                internalNodes.append(internalNode);
                internalNodeMap[internalObj["name"].toString()] = internalNode;
                // 内部节点不添加到场景，只保存在组节点中
            }
            groupNode->setInternalNodes(internalNodes);
            
            // 恢复内部连接
            QJsonArray internalConnsArray = nodeObj["internalConnections"].toArray();
            QList<Connection*> internalConnections;
            
            for (const QJsonValue &connValue : internalConnsArray) {
                QJsonObject connObj = connValue.toObject();
                QString fromName = connObj["fromNode"].toString();
                QString toName = connObj["toNode"].toString();
                
                Node *fromNode = internalNodeMap.value(fromName);
                Node *toNode = internalNodeMap.value(toName);
                
                if (fromNode && toNode) {
                    int fromPort = connObj["fromPort"].toInt(0);
                    int toPort = connObj["toPort"].toInt(0);
                    Connection *conn = new Connection(fromNode, fromPort, toNode, toPort);
                    internalConnections.append(conn);
                }
            }
            groupNode->setInternalConnections(internalConnections);
            
            // 恢复原始位置
            QJsonArray origPosArray = nodeObj["originalPositions"].toArray();
            QMap<Node*, QPointF> originalPositions;
            for (const QJsonValue &posValue : origPosArray) {
                QJsonObject posObj = posValue.toObject();
                QString nodeName = posObj["nodeName"].toString();
                Node *internalNode = internalNodeMap.value(nodeName);
                if (internalNode) {
                    originalPositions[internalNode] = QPointF(posObj["x"].toDouble(), posObj["y"].toDouble());
                }
            }
            groupNode->setOriginalPositions(originalPositions);
            
            // 计算端口映射
            groupNode->calculatePortMappings();
            
            node = groupNode;
        } else {
            // 普通节点
            node = Node::fromJson(nodeObj);
        }
        
        if (node) {
            addItem(node);
            m_nodes.append(node);
            nodeMap[nodeObj["id"].toString()] = node;
        }
    }
    
    // 然后创建连接（支持新旧格式）
    QJsonArray connectionsArray = data["connections"].toArray();
    for (const QJsonValue &connValue : connectionsArray) {
        QJsonObject connObj = connValue.toObject();
        
        // 支持两种格式的节点ID字段
        QString fromNodeId = connObj.contains("from") ? 
            connObj["from"].toString() : connObj["fromNode"].toString();
        QString toNodeId = connObj.contains("to") ? 
            connObj["to"].toString() : connObj["toNode"].toString();
        
        Node *fromNode = nodeMap[fromNodeId];
        Node *toNode = nodeMap[toNodeId];
        
        if (fromNode && toNode) {
            // 获取端口索引（如果存在），默认为0
            int fromPort = connObj["fromPort"].toInt(0);
            int toPort = connObj["toPort"].toInt(0);
            
            // 创建连接
            Connection *connection = new Connection(fromNode, fromPort, toNode, toPort);
            
            // 加载连线类型（如果存在）
            if (connObj.contains("lineType")) {
                connection->setLineType(static_cast<Connection::LineType>(connObj["lineType"].toInt(0)));
            }
            
            addItem(connection);
            m_connections.append(connection);
        }
    }
}

void NodeScene::deleteSelected()
{
    QList<QGraphicsItem*> selected = this->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    
    // 分别收集要删除的节点和连接
    QList<Node*> nodesToDelete;
    QList<Connection*> connectionsToDelete;
    
    for (QGraphicsItem *item : selected) {
        if (Node *node = dynamic_cast<Node*>(item)) {
            nodesToDelete.append(node);
        } else if (Connection *conn = dynamic_cast<Connection*>(item)) {
            connectionsToDelete.append(conn);
        }
    }
    
    // 收集与要删除节点相关的所有连接
    for (Node *node : nodesToDelete) {
        for (Connection *conn : node->getConnections()) {
            if (!connectionsToDelete.contains(conn)) {
                connectionsToDelete.append(conn);
            }
        }
    }
    
    if (nodesToDelete.isEmpty() && connectionsToDelete.isEmpty()) {
        return;
    }
    
    // 使用撤销命令
    DeleteCommand *cmd = new DeleteCommand(this, nodesToDelete, connectionsToDelete);
    m_undoStack.push(cmd);
}

QGraphicsItem* NodeScene::getSelectedNode() const
{
    QList<QGraphicsItem*> selected = selectedItems();
    for (QGraphicsItem *item : selected) {
        if (dynamic_cast<Node*>(item)) {
            return item;
        }
    }
    return nullptr;
}

void NodeScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (DEBUG_CONNECTION) {
        qDebug() << "=== NodeScene::mousePressEvent ===";
        qDebug() << "鼠标位置:" << event->scenePos();
        qDebug() << "当前连接状态:" << m_connectionState;
    }
    
    if (event->button() == Qt::LeftButton) {
        // 获取所有在点击位置的项
        QList<QGraphicsItem*> items = this->items(event->scenePos());
        Node* clickedNode = nullptr;
        
        // 遍历所有图形项，查找节点
        for (QGraphicsItem* graphicsItem : items) {
            if (Node *node = dynamic_cast<Node*>(graphicsItem)) {
                clickedNode = node;
                if (DEBUG_CONNECTION) {
                    qDebug() << "找到节点:" << node->getName();
                }
                break;
            }
        }
        
        if (clickedNode) {
            // 检查是否点击了输出端口，开始拖拽连线
            int outputPortIndex = clickedNode->getOutputPortIndexAt(event->scenePos());
            
            if (outputPortIndex >= 0) {
                if (DEBUG_CONNECTION) qDebug() << "开始拖拽连线 - 从输出端口" << outputPortIndex;
                m_tempFromNode = clickedNode;
                m_tempFromPortIndex = outputPortIndex;
                m_connectionState = FromNodeClicked;
                
                // 创建临时连接线（从指定端口开始）
                QPointF startPos = clickedNode->getOutputPortPos(outputPortIndex);
                m_tempLine = new QGraphicsLineItem(QLineF(startPos, event->scenePos()));
                m_tempLine->setPen(QPen(Qt::cyan, 3, Qt::DashLine));
                addItem(m_tempLine);
                if (DEBUG_CONNECTION) qDebug() << "创建临时连接线完成";
                return; // 阻止默认处理，开始拖拽连线
            }
        }
        
        // 如果不是从输出端口开始，则进行正常的节点选择操作
        if (DEBUG_CONNECTION) qDebug() << "进行正常节点选择操作";
        QGraphicsScene::mousePressEvent(event);
        emit selectionChanged(selectedItems().isEmpty() ? nullptr : selectedItems().first());
    } else if (event->button() == Qt::RightButton) {
        if (DEBUG_CONNECTION) qDebug() << "右键点击 - 取消连接操作";
        // 右键取消连接操作
        if (m_connectionState != None) {
            cancelConnection();
        }
    }
}

void NodeScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (DEBUG_CONNECTION && (m_connectionState != None || m_tempLine)) {
        qDebug() << "=== mouseMoveEvent ===";
        qDebug() << "鼠标位置:" << event->scenePos();
        qDebug() << "连接状态:" << m_connectionState;
        qDebug() << "临时线存在:" << (m_tempLine != nullptr);
    }
    
    // 更新临时连接线位置（如果在连接过程中）
    if (m_tempLine && m_tempFromNode) {
        QPointF startPos = m_tempFromNode->getOutputPortPos(m_tempFromPortIndex);
        QPointF endPos = event->scenePos();
        
        // 检查是否靠近某个输入端口，如果是则吸附到该端口
        QList<QGraphicsItem*> items = this->items(event->scenePos());
        for (QGraphicsItem* graphicsItem : items) {
            if (Node *node = dynamic_cast<Node*>(graphicsItem)) {
                if (node != m_tempFromNode) {
                    int inputPort = node->getInputPortIndexAt(event->scenePos());
                    if (inputPort >= 0) {
                        // 吸附到输入端口位置
                        endPos = node->getInputPortPos(inputPort);
                        // 改变临时线颜色表示可以连接
                        m_tempLine->setPen(QPen(Qt::green, 3, Qt::DashLine));
                        break;
                    }
                }
            }
        }
        
        // 如果没有吸附，恢复默认颜色
        if (endPos == event->scenePos()) {
            m_tempLine->setPen(QPen(Qt::cyan, 3, Qt::DashLine));
        }
        
        QLineF newLine(startPos, endPos);
        m_tempLine->setLine(newLine);
        if (DEBUG_CONNECTION) qDebug() << "更新临时连接线位置，从端口" << m_tempFromPortIndex;
    }
    
    // 始终更新端口高亮状态
    updatePortHighlights(event->scenePos());
    
    QGraphicsScene::mouseMoveEvent(event);
}

void NodeScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (DEBUG_CONNECTION) {
        qDebug() << "=== NodeScene::mouseReleaseEvent ===";
        qDebug() << "鼠标位置:" << event->scenePos();
        qDebug() << "连接状态:" << m_connectionState;
    }
    
    // 处理拖拽连线的释放
    if (event->button() == Qt::LeftButton && m_connectionState == FromNodeClicked) {
        // 查找释放位置附近的输入端口
        Node* targetNode = nullptr;
        int targetPortIndex = -1;
        
        // 获取释放位置附近的所有节点
        QList<QGraphicsItem*> items = this->items(event->scenePos());
        for (QGraphicsItem* graphicsItem : items) {
            if (Node *node = dynamic_cast<Node*>(graphicsItem)) {
                // 不能连接到自己
                if (node != m_tempFromNode) {
                    int inputPort = node->getInputPortIndexAt(event->scenePos());
                    if (inputPort >= 0) {
                        targetNode = node;
                        targetPortIndex = inputPort;
                        break;
                    }
                }
            }
        }
        
        // 如果找到了有效的目标端口，完成连线
        if (targetNode && targetPortIndex >= 0) {
            if (DEBUG_CONNECTION) {
                qDebug() << "完成连线 - 吸附到输入端口";
                qDebug() << "源节点:" << m_tempFromNode->getName() << "端口:" << m_tempFromPortIndex;
                qDebug() << "目标节点:" << targetNode->getName() << "端口:" << targetPortIndex;
            }
            
            // 创建连接
            addConnection(m_tempFromNode, m_tempFromPortIndex, targetNode, targetPortIndex);
            
            // 清理临时状态
            cleanupTempConnection();
            
            if (DEBUG_CONNECTION) qDebug() << "连线创建成功";
        } else {
            // 没有找到有效目标，取消连线
            if (DEBUG_CONNECTION) qDebug() << "未找到有效目标端口，取消连线";
            cancelConnection();
        }
        
        return;
    }
    
    // 正常的节点拖拽释放
    QGraphicsScene::mouseReleaseEvent(event);
}

/**
 * @brief 取消当前连线操作
 */
void NodeScene::cancelConnection()
{
    if (DEBUG_CONNECTION) qDebug() << "取消连线操作";
    
    cleanupTempConnection();
    clearPortHighlights();
}

/**
 * @brief 清理临时连线状态
 */
void NodeScene::cleanupTempConnection()
{
    if (m_tempLine) {
        removeItem(m_tempLine);
        delete m_tempLine;
        m_tempLine = nullptr;
    }
    m_tempFromNode = nullptr;
    m_tempFromPortIndex = 0;
    m_connectionState = None;
}

/**
 * @brief 清除所有端口高亮
 */
void NodeScene::clearPortHighlights()
{
    for (Node *node : m_nodes) {
        node->setInputPortHighlighted(false);
        node->setOutputPortHighlighted(false);
    }
}

/**
 * @brief 更新端口高亮状态
 * @param mousePos 鼠标当前位置
 */
void NodeScene::updatePortHighlights(const QPointF &mousePos)
{
    if (DEBUG_CONNECTION) {
        qDebug() << "=== updatePortHighlights ===";
        qDebug() << "鼠标位置:" << mousePos;
        qDebug() << "节点总数:" << m_nodes.size();
    }
    
    // 首先清除所有高亮
    clearPortHighlights();
    
    // 根据当前连接状态决定高亮哪些端口
    if (m_connectionState == FromNodeClicked) {
        if (DEBUG_CONNECTION) qDebug() << "连接模式：高亮输入端口";
        // 如果已点击源节点，高亮所有节点的输入端口
        for (Node *node : m_nodes) {
            if (node != m_tempFromNode) {  // 不能连接到自己
                bool isAtInputPort = node->isPointAtInputPort(mousePos);
                if (DEBUG_CONNECTION) {
                    qDebug() << "检查节点" << node->getName() << "输入端口:" << isAtInputPort;
                }
                if (isAtInputPort) {
                    node->setInputPortHighlighted(true);
                    if (DEBUG_CONNECTION) qDebug() << "高亮输入端口:" << node->getName();
                }
            }
        }
    } else {
        if (DEBUG_CONNECTION) qDebug() << "空闲模式：高亮输出端口";
        // 检查鼠标是否在某个节点的输出端口附近
        for (Node *node : m_nodes) {
            bool isAtOutputPort = node->isPointAtOutputPort(mousePos);
            if (DEBUG_CONNECTION) {
                qDebug() << "检查节点" << node->getName() << "输出端口:" << isAtOutputPort;
            }
            if (isAtOutputPort) {
                node->setOutputPortHighlighted(true);
                if (DEBUG_CONNECTION) qDebug() << "高亮输出端口:" << node->getName();
                break;  // 一次只高亮一个输出端口
            }
        }
    }
}

/**
 * @brief 当节点模板更新时更新场景中的相应节点
 * @param typeId 更新的模板类型ID
 * 
 * 遍历场景中的所有节点，找到匹配类型的节点并更新其属性
 */
void NodeScene::onTemplateUpdated(const QString &typeId)
{
    // 获取更新后的模板
    NodeTemplate tmpl = NodeLibrary::instance()->getTemplate(typeId);
    if (!tmpl.isValid()) {
        return;
    }
    
    // 遍历所有节点，更新匹配类型的节点
    for (Node *node : m_nodes) {
        if (node->getType() == typeId) {
            // 更新节点的显示属性
            node->setCustomColor(tmpl.getColor());
            node->setDisplayTypeName(tmpl.getDisplayName());
            node->setInputPortCount(tmpl.getInputPortCount());
            node->setOutputPortCount(tmpl.getOutputPortCount());
            
            if (DEBUG_CONNECTION) {
                qDebug() << "更新节点:" << node->getName() 
                         << "类型:" << typeId
                         << "颜色:" << tmpl.getColor().name()
                         << "输入端口:" << tmpl.getInputPortCount()
                         << "输出端口:" << tmpl.getOutputPortCount();
            }
        }
    }
    
    // 更新场景显示
    update();
}

/**
 * @brief 复制选中的元素到剪贴板
 */
void NodeScene::copySelected()
{
    QList<QGraphicsItem*> selected = selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    
    // 收集选中的节点
    QList<Node*> selectedNodes;
    QMap<Node*, QString> nodeIdMap;  // 节点到临时ID的映射
    int nodeIndex = 0;
    
    for (QGraphicsItem *item : selected) {
        if (Node *node = dynamic_cast<Node*>(item)) {
            selectedNodes.append(node);
            nodeIdMap[node] = QString("copy_node_%1").arg(nodeIndex++);
        }
    }
    
    if (selectedNodes.isEmpty()) {
        return;
    }
    
    // 计算选中节点的中心点
    QPointF center(0, 0);
    for (Node *node : selectedNodes) {
        center += node->pos();
    }
    center /= selectedNodes.size();
    
    // 序列化节点
    QJsonArray nodesArray;
    for (Node *node : selectedNodes) {
        QJsonObject nodeObj = node->toJson();
        nodeObj["copyId"] = nodeIdMap[node];
        // 存储相对于中心点的位置
        nodeObj["relX"] = node->x() - center.x();
        nodeObj["relY"] = node->y() - center.y();
        nodesArray.append(nodeObj);
    }
    
    // 序列化选中节点之间的连接
    QJsonArray connectionsArray;
    for (Connection *conn : m_connections) {
        Node *fromNode = conn->getFromNode();
        Node *toNode = conn->getToNode();
        
        // 只复制两端都在选中节点中的连接
        if (selectedNodes.contains(fromNode) && selectedNodes.contains(toNode)) {
            QJsonObject connObj;
            connObj["fromCopyId"] = nodeIdMap[fromNode];
            connObj["toCopyId"] = nodeIdMap[toNode];
            connObj["fromPort"] = conn->getFromPortIndex();
            connObj["toPort"] = conn->getToPortIndex();
            connObj["lineType"] = static_cast<int>(conn->getLineType());
            connectionsArray.append(connObj);
        }
    }
    
    // 存储到剪贴板
    m_clipboard = QJsonObject();
    m_clipboard["nodes"] = nodesArray;
    m_clipboard["connections"] = connectionsArray;
    
    qDebug() << "复制了" << selectedNodes.size() << "个节点和" 
             << connectionsArray.size() << "条连接";
}

/**
 * @brief 粘贴剪贴板中的元素
 * @param offset 粘贴位置偏移量
 */
void NodeScene::paste(const QPointF &offset)
{
    if (m_clipboard.isEmpty()) {
        return;
    }
    
    // 清除当前选择
    clearSelection();
    
    // 使用撤销命令
    PasteCommand *cmd = new PasteCommand(this, m_clipboard, offset);
    m_undoStack.push(cmd);
}

/**
 * @brief 剪切选中的元素
 */
void NodeScene::cutSelected()
{
    copySelected();
    deleteSelected();
}

/**
 * @brief 全选所有元素
 */
void NodeScene::selectAll()
{
    for (QGraphicsItem *item : items()) {
        item->setSelected(true);
    }
}

/**
 * @brief 检查是否有可粘贴的内容
 */
bool NodeScene::canPaste() const
{
    return !m_clipboard.isEmpty() && m_clipboard.contains("nodes");
}

/**
 * @brief 检查是否可以打包选中的节点
 * @return 如果有至少两个选中的节点返回true（支持多层打包）
 */
bool NodeScene::canGroup() const
{
    QList<QGraphicsItem*> selected = selectedItems();
    int nodeCount = 0;
    
    for (QGraphicsItem *item : selected) {
        if (dynamic_cast<Node*>(item)) {
            // 允许任何节点（包括组节点）参与打包，支持多层嵌套
            nodeCount++;
        }
    }
    
    return nodeCount >= 2;
}

/**
 * @brief 检查是否可以拆分选中的组节点
 * @return 如果选中的是组节点返回true
 */
bool NodeScene::canUngroup() const
{
    QList<QGraphicsItem*> selected = selectedItems();
    
    for (QGraphicsItem *item : selected) {
        if (dynamic_cast<GroupNode*>(item)) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 将选中的节点打包为组节点
 * @return 如果成功打包返回true
 * 
 * 支持多层打包：组节点也可以被打包进更大的组节点
 */
bool NodeScene::groupSelected()
{
    if (!canGroup()) {
        return false;
    }
    
    QList<QGraphicsItem*> selected = selectedItems();
    
    // 收集选中的所有节点（包括组节点，支持多层打包）
    QList<Node*> nodesToGroup;
    QSet<Node*> nodeSet;
    
    for (QGraphicsItem *item : selected) {
        if (Node *node = dynamic_cast<Node*>(item)) {
            // 允许任何节点参与打包，包括组节点
            nodesToGroup.append(node);
            nodeSet.insert(node);
        }
    }
    
    if (nodesToGroup.size() < 2) {
        return false;
    }
    
    // 分类连接：内部连接 vs 外部连接
    QList<Connection*> internalConnections;
    QList<ExternalConnection> externalConnections;
    
    for (Connection *conn : m_connections) {
        Node *fromNode = conn->getFromNode();
        Node *toNode = conn->getToNode();
        
        bool fromInGroup = nodeSet.contains(fromNode);
        bool toInGroup = nodeSet.contains(toNode);
        
        if (fromInGroup && toInGroup) {
            // 两端都在组内：内部连接
            internalConnections.append(conn);
        } else if (fromInGroup && !toInGroup) {
            // 从组内连接到组外：输出
            ExternalConnection ext;
            ext.externalNode = toNode;
            ext.externalPortIndex = conn->getToPortIndex();
            ext.internalNode = fromNode;
            ext.internalPortIndex = conn->getFromPortIndex();
            ext.isInput = false;  // 对于组节点来说是输出
            ext.originalConnection = conn;
            externalConnections.append(ext);
        } else if (!fromInGroup && toInGroup) {
            // 从组外连接到组内：输入
            ExternalConnection ext;
            ext.externalNode = fromNode;
            ext.externalPortIndex = conn->getFromPortIndex();
            ext.internalNode = toNode;
            ext.internalPortIndex = conn->getToPortIndex();
            ext.isInput = true;  // 对于组节点来说是输入
            ext.originalConnection = conn;
            externalConnections.append(ext);
        }
    }
    
    // 创建撤销命令
    GroupNodesCommand *cmd = new GroupNodesCommand(this, nodesToGroup, 
                                                    internalConnections, externalConnections);
    m_undoStack.push(cmd);
    
    // 选中新创建的组节点
    clearSelection();
    if (cmd->getGroupNode()) {
        cmd->getGroupNode()->setSelected(true);
    }
    
    qDebug() << "打包了" << nodesToGroup.size() << "个节点";
    qDebug() << "内部连接:" << internalConnections.size() << "外部连接:" << externalConnections.size();
    
    return true;
}

/**
 * @brief 拆分选中的组节点
 * @return 如果成功拆分返回true
 */
bool NodeScene::ungroupSelected()
{
    if (!canUngroup()) {
        return false;
    }
    
    QList<QGraphicsItem*> selected = selectedItems();
    
    // 查找选中的组节点
    GroupNode *groupNode = nullptr;
    for (QGraphicsItem *item : selected) {
        if (GroupNode *group = dynamic_cast<GroupNode*>(item)) {
            groupNode = group;
            break;
        }
    }
    
    if (!groupNode) {
        return false;
    }
    
    // 创建撤销命令
    UngroupNodesCommand *cmd = new UngroupNodesCommand(this, groupNode);
    m_undoStack.push(cmd);
    
    // 清除选择
    clearSelection();
    
    // 选中拆分后的节点
    for (Node *node : groupNode->getInternalNodes()) {
        node->setSelected(true);
    }
    
    qDebug() << "拆分组节点，恢复了" << groupNode->getInternalNodes().size() << "个节点";
    
    return true;
}

/**
 * @file GroupNode.cpp
 * @brief 组节点类实现文件
 * @author
 * @version 1.0.0
 * @date 2024
 */

#include "GroupNode.h"
#include <QPainter>
#include <QJsonArray>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QFontMetrics>
#include <QSet>

GroupNode::GroupNode(const QString &name, const QPointF &position)
    : Node("group", name, position)
    , m_groupLevel(1)  // 默认1级
{
    // 设置组节点特有的颜色
    setCustomColor(QColor(100, 149, 237));  // 康乃馨蓝
    setDisplayTypeName("组合节点");
}

GroupNode::~GroupNode()
{
    // 注意：内部节点和连接的生命周期由场景管理
    // 这里不删除它们，只清空列表
    m_internalNodes.clear();
    m_internalConnections.clear();
}

void GroupNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // 先调用基类绘制（基类会调用虚函数 drawPorts，自动调用 GroupNode::drawPorts）
    Node::paint(painter, option, widget);
    
    // 在节点上绘制组标识（双层边框效果）
    painter->setRenderHint(QPainter::Antialiasing);
    
    QRectF rect = boundingRect().adjusted(PORT_RADIUS + 2, 2, -PORT_RADIUS - 2, -2);
    
    // 绘制内部虚线边框表示这是一个组
    QPen dashedPen(QColor(255, 255, 255, 150), 1, Qt::DashLine);
    painter->setPen(dashedPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(rect.adjusted(4, 4, -4, -4), 4, 4);
    
    // 在右上角绘制组图标
    QRectF iconRect(rect.right() - 20, rect.top() + 4, 16, 16);
    painter->setPen(QPen(Qt::white, 1));
    painter->setBrush(QColor(255, 255, 255, 100));
    
    // 绘制层叠矩形图标表示组
    painter->drawRect(iconRect.adjusted(2, 2, 0, 0));
    painter->drawRect(iconRect.adjusted(0, 0, -2, -2));
    
    // 在左上角绘制组件等级标签
    QString levelText = QString("Lv.%1").arg(m_groupLevel);
    QFont levelFont = painter->font();
    levelFont.setPointSize(8);
    levelFont.setBold(true);
    painter->setFont(levelFont);
    
    QFontMetrics fm(levelFont);
    int textWidth = fm.horizontalAdvance(levelText) + 6;
    int textHeight = fm.height() + 2;
    
    QRectF levelRect(rect.left() + 4, rect.top() + 4, textWidth, textHeight);
    
    // 绘制等级背景（根据等级显示不同颜色）
    QColor levelColor;
    switch (m_groupLevel) {
        case 1: levelColor = QColor(100, 149, 237); break;  // 蓝色
        case 2: levelColor = QColor(50, 205, 50); break;    // 绿色
        case 3: levelColor = QColor(255, 165, 0); break;    // 橙色
        case 4: levelColor = QColor(255, 69, 0); break;     // 红橙色
        case 5: levelColor = QColor(148, 0, 211); break;    // 紫色
        default: levelColor = QColor(255, 215, 0); break;   // 金色（高等级）
    }
    
    painter->setBrush(levelColor);
    painter->setPen(QPen(levelColor.darker(120), 1));
    painter->drawRoundedRect(levelRect, 3, 3);
    
    // 绘制等级文字
    painter->setPen(Qt::white);
    painter->drawText(levelRect, Qt::AlignCenter, levelText);
}

void GroupNode::setGroupLevel(int level)
{
    if (level < 1) level = 1;
    if (level > 99) level = 99;
    m_groupLevel = level;
    update();  // 触发重绘
}

void GroupNode::drawPorts(QPainter *painter)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    
    qreal width = getWidth();
    qreal height = getHeight();
    
    // 设置字体
    QFont labelFont = painter->font();
    labelFont.setPointSize(7);
    labelFont.setBold(true);
    painter->setFont(labelFont);
    
    // 计算输入端口数量
    int inputCount = m_inputPortMappings.size();
    if (inputCount == 0) inputCount = 1;  // 至少显示一个端口
    
    // 计算输出端口数量
    int outputCount = m_outputPortMappings.size();
    if (outputCount == 0) outputCount = 1;
    
    // 绘制输入端口及标签
    for (int i = 0; i < m_inputPortMappings.size(); ++i) {
        // 使用与 Node::getInputPortPos 相同的计算方式
        QPointF portPos = getInputPortPos(i);
        QPointF portCenter = portPos - pos();  // 转换为本地坐标
        
        // 绘制端口圆形
        QRadialGradient gradient(portCenter, PORT_RADIUS);
        gradient.setColorAt(0, QColor(150, 200, 255, 255));
        gradient.setColorAt(0.8, QColor(100, 150, 220, 255));
        gradient.setColorAt(1, QColor(50, 100, 180, 255));
        painter->setBrush(gradient);
        painter->setPen(QPen(QColor(30, 80, 150, 255), 1.5));
        painter->drawEllipse(portCenter, PORT_RADIUS, PORT_RADIUS);
        
        // 绘制端口来源标签（在端口右侧，带背景框）
        QString label = m_inputPortMappings[i].portLabel;
        QFontMetrics fm(labelFont);
        int textWidth = fm.horizontalAdvance(label) + 6;
        int textHeight = fm.height() + 2;
        
        QRectF labelRect(portCenter.x() + PORT_RADIUS + 4, 
                        portCenter.y() - textHeight/2, 
                        textWidth, textHeight);
        
        // 绘制标签背景
        painter->setBrush(QColor(40, 60, 100, 200));
        painter->setPen(QPen(QColor(100, 150, 220), 1));
        painter->drawRoundedRect(labelRect, 3, 3);
        
        // 绘制标签文字
        painter->setPen(QColor(220, 240, 255));
        painter->drawText(labelRect, Qt::AlignCenter, label);
    }
    
    // 绘制输出端口及标签
    for (int i = 0; i < m_outputPortMappings.size(); ++i) {
        // 使用与 Node::getOutputPortPos 相同的计算方式
        QPointF portPos = getOutputPortPos(i);
        QPointF portCenter = portPos - pos();  // 转换为本地坐标
        
        // 绘制端口圆形
        QRadialGradient gradient(portCenter, PORT_RADIUS);
        gradient.setColorAt(0, QColor(255, 220, 150, 255));
        gradient.setColorAt(0.8, QColor(255, 180, 100, 255));
        gradient.setColorAt(1, QColor(220, 140, 50, 255));
        painter->setBrush(gradient);
        painter->setPen(QPen(QColor(180, 100, 30, 255), 1.5));
        painter->drawEllipse(portCenter, PORT_RADIUS, PORT_RADIUS);
        
        // 绘制端口来源标签（在端口左侧，带背景框）
        QString label = m_outputPortMappings[i].portLabel;
        QFontMetrics fm(labelFont);
        int textWidth = fm.horizontalAdvance(label) + 6;
        int textHeight = fm.height() + 2;
        
        QRectF labelRect(portCenter.x() - PORT_RADIUS - 4 - textWidth, 
                        portCenter.y() - textHeight/2, 
                        textWidth, textHeight);
        
        // 绘制标签背景
        painter->setBrush(QColor(100, 70, 30, 200));
        painter->setPen(QPen(QColor(220, 160, 80), 1));
        painter->drawRoundedRect(labelRect, 3, 3);
        
        // 绘制标签文字
        painter->setPen(QColor(255, 240, 210));
        painter->drawText(labelRect, Qt::AlignCenter, label);
    }
    
    painter->restore();
}

void GroupNode::setInternalNodes(const QList<Node*> &nodes)
{
    m_internalNodes = nodes;
}

void GroupNode::setInternalConnections(const QList<Connection*> &connections)
{
    m_internalConnections = connections;
}

void GroupNode::setExternalConnections(const QList<ExternalConnection> &connections)
{
    m_externalConnections = connections;
}

void GroupNode::setOriginalPositions(const QMap<Node*, QPointF> &positions)
{
    m_originalPositions = positions;
}

void GroupNode::calculatePortMappings()
{
    m_inputPortMappings.clear();
    m_outputPortMappings.clear();
    
    // 收集所有内部节点
    QSet<Node*> internalNodeSet;
    for (Node *node : m_internalNodes) {
        internalNodeSet.insert(node);
    }
    
    // 记录已经有连接的端口（内部连接）
    // key: (node, portIndex, isInput)
    QSet<QString> connectedPorts;
    
    // 遍历内部连接，标记已连接的端口
    for (Connection *conn : m_internalConnections) {
        Node *fromNode = conn->getFromNode();
        Node *toNode = conn->getToNode();
        int fromPort = conn->getFromPortIndex();
        int toPort = conn->getToPortIndex();
        
        // 标记输出端口和输入端口为已连接
        QString fromKey = QString("%1_%2_out").arg(reinterpret_cast<quintptr>(fromNode)).arg(fromPort);
        QString toKey = QString("%1_%2_in").arg(reinterpret_cast<quintptr>(toNode)).arg(toPort);
        connectedPorts.insert(fromKey);
        connectedPorts.insert(toKey);
    }
    
    // 遍历外部连接，添加到端口映射并标记
    for (const ExternalConnection &extConn : m_externalConnections) {
        PortMapping mapping;
        mapping.internalNode = extConn.internalNode;
        mapping.internalPortIndex = extConn.internalPortIndex;
        mapping.isInput = extConn.isInput;
        mapping.portLabel = QString("%1[%2]").arg(extConn.internalNode->getName())
                                              .arg(extConn.internalPortIndex);
        
        // 标记端口为已连接
        QString key;
        if (extConn.isInput) {
            key = QString("%1_%2_in").arg(reinterpret_cast<quintptr>(extConn.internalNode))
                                     .arg(extConn.internalPortIndex);
            m_inputPortMappings.append(mapping);
        } else {
            key = QString("%1_%2_out").arg(reinterpret_cast<quintptr>(extConn.internalNode))
                                      .arg(extConn.internalPortIndex);
            m_outputPortMappings.append(mapping);
        }
        connectedPorts.insert(key);
    }
    
    // 遍历所有内部节点，找出完全悬空的端口（没有任何连接）
    for (Node *node : m_internalNodes) {
        // 检查输入端口
        for (int i = 0; i < node->getInputPortCount(); ++i) {
            QString key = QString("%1_%2_in").arg(reinterpret_cast<quintptr>(node)).arg(i);
            if (!connectedPorts.contains(key)) {
                // 这是一个悬空的输入端口
                PortMapping mapping;
                mapping.internalNode = node;
                mapping.internalPortIndex = i;
                mapping.isInput = true;
                mapping.portLabel = QString("%1[%2]").arg(node->getName()).arg(i);
                m_inputPortMappings.append(mapping);
            }
        }
        
        // 检查输出端口
        for (int i = 0; i < node->getOutputPortCount(); ++i) {
            QString key = QString("%1_%2_out").arg(reinterpret_cast<quintptr>(node)).arg(i);
            if (!connectedPorts.contains(key)) {
                // 这是一个悬空的输出端口
                PortMapping mapping;
                mapping.internalNode = node;
                mapping.internalPortIndex = i;
                mapping.isInput = false;
                mapping.portLabel = QString("%1[%2]").arg(node->getName()).arg(i);
                m_outputPortMappings.append(mapping);
            }
        }
    }
    
    // 更新端口数量
    int inputCount = qMax(1, m_inputPortMappings.size());
    int outputCount = qMax(1, m_outputPortMappings.size());
    setInputPortCount(inputCount);
    setOutputPortCount(outputCount);
    
    // 根据端口数量自动调整节点大小
    int maxPorts = qMax(inputCount, outputCount);
    qreal minHeight = 50 + maxPorts * 20;  // 每个端口需要约20像素的空间
    qreal currentHeight = getHeight();
    if (minHeight > currentHeight) {
        setSize(getWidth(), minHeight);
    }
    
    // 确保宽度足够显示标签
    qreal minWidth = 150;  // 组节点需要更宽以显示标签
    if (getWidth() < minWidth) {
        setSize(minWidth, getHeight());
    }
}

QJsonObject GroupNode::toJson() const
{
    QJsonObject json = Node::toJson();
    json["isGroup"] = true;
    json["groupLevel"] = m_groupLevel;  // 保存组件等级
    
    // 保存内部节点
    QJsonArray nodesArray;
    for (Node *node : m_internalNodes) {
        nodesArray.append(node->toJson());
    }
    json["internalNodes"] = nodesArray;
    
    // 保存内部连接
    QJsonArray connectionsArray;
    for (Connection *conn : m_internalConnections) {
        connectionsArray.append(conn->toJson());
    }
    json["internalConnections"] = connectionsArray;
    
    // 保存原始位置
    QJsonArray positionsArray;
    for (auto it = m_originalPositions.begin(); it != m_originalPositions.end(); ++it) {
        QJsonObject posObj;
        posObj["nodeName"] = it.key()->getName();
        posObj["x"] = it.value().x();
        posObj["y"] = it.value().y();
        positionsArray.append(posObj);
    }
    json["originalPositions"] = positionsArray;
    
    // 保存端口映射
    QJsonArray inputMappings;
    for (const PortMapping &pm : m_inputPortMappings) {
        QJsonObject pmObj;
        pmObj["nodeName"] = pm.internalNode->getName();
        pmObj["portIndex"] = pm.internalPortIndex;
        pmObj["label"] = pm.portLabel;
        inputMappings.append(pmObj);
    }
    json["inputPortMappings"] = inputMappings;
    
    QJsonArray outputMappings;
    for (const PortMapping &pm : m_outputPortMappings) {
        QJsonObject pmObj;
        pmObj["nodeName"] = pm.internalNode->getName();
        pmObj["portIndex"] = pm.internalPortIndex;
        pmObj["label"] = pm.portLabel;
        outputMappings.append(pmObj);
    }
    json["outputPortMappings"] = outputMappings;
    
    return json;
}

GroupNode* GroupNode::fromJson(const QJsonObject &json, const QMap<QString, Node*> &allNodes)
{
    QString name = json["name"].toString();
    QPointF position(json["x"].toDouble(), json["y"].toDouble());
    
    GroupNode *group = new GroupNode(name, position);
    
    // 恢复基本属性
    if (json.contains("customColor")) {
        group->setCustomColor(QColor(json["customColor"].toString()));
    }
    
    // 恢复组件等级
    if (json.contains("groupLevel")) {
        group->setGroupLevel(json["groupLevel"].toInt(1));
    }
    
    // 内部节点和连接需要在外部重建后设置
    // 这里只返回基本的组节点
    
    return group;
}


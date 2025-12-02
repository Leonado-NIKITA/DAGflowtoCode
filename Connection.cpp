/**
 * @file Connection.cpp
 * @brief 连接线类实现文件
 * @author 
 * @version 1.0.0
 * @date 2024
 */

#include "Connection.h"
#include "Node.h"

/**
 * @brief 构造函数，创建两个节点之间的连接（兼容旧版本，使用第一个端口）
 * @param fromNode 源节点
 * @param toNode 目标节点
 */
Connection::Connection(Node *fromNode, Node *toNode)
    : m_fromNode(fromNode)
    , m_toNode(toNode)
    , m_fromPortIndex(0)
    , m_toPortIndex(0)
    , m_lineType(Bezier)
{
    initConnection();
}

/**
 * @brief 构造函数，创建两个节点指定端口之间的连接
 * @param fromNode 源节点
 * @param fromPortIndex 源节点的输出端口索引
 * @param toNode 目标节点
 * @param toPortIndex 目标节点的输入端口索引
 */
Connection::Connection(Node *fromNode, int fromPortIndex, Node *toNode, int toPortIndex)
    : m_fromNode(fromNode)
    , m_toNode(toNode)
    , m_fromPortIndex(fromPortIndex)
    , m_toPortIndex(toPortIndex)
    , m_lineType(Bezier)
{
    initConnection();
}

/**
 * @brief 初始化连接
 */
void Connection::initConnection()
{
    if (DEBUG_CONNECTION_PATH) {
        qDebug() << "=== Connection构造函数 ===";
        qDebug() << "源节点:" << (m_fromNode ? m_fromNode->getName() : "nullptr")
                 << "端口:" << m_fromPortIndex;
        qDebug() << "目标节点:" << (m_toNode ? m_toNode->getName() : "nullptr")
                 << "端口:" << m_toPortIndex;
    }
    
    // 设置图形属性
    setZValue(-1);                    // 设置Z值为-1，确保连接线显示在节点下方
    setPen(QPen(Qt::yellow, 3));      // 设置连接线样式：黄色，宽度3像素
    setBrush(Qt::NoBrush);            // 不使用填充
    setFlag(QGraphicsItem::ItemIsSelectable, true);  // 允许选择连接线
    
    // 建立与节点的双向关联
    if (m_fromNode) m_fromNode->addConnection(this);
    if (m_toNode) m_toNode->addConnection(this);
    
    // 初始化连接线路径
    updatePath();
}

/**
 * @brief 析构函数，清理相关资源
 */
Connection::~Connection()
{
    // 从两个节点的连接列表中移除此连接线
    if (m_fromNode) m_fromNode->removeConnection(this);
    if (m_toNode) m_toNode->removeConnection(this);
}

/**
 * @brief 设置连线类型
 * @param type 连线类型
 */
void Connection::setLineType(LineType type)
{
    if (m_lineType != type) {
        m_lineType = type;
        updatePath();  // 重新绘制路径
    }
}

/**
 * @brief 获取线型的显示名称
 * @param type 线型枚举值
 * @return 线型的中文名称
 */
QString Connection::lineTypeName(LineType type)
{
    switch (type) {
    case Bezier:     return "贝塞尔曲线";
    case Straight:   return "直线";
    case Orthogonal: return "直角线";
    default:         return "未知";
    }
}

/**
 * @brief 更新连接线的路径
 * 
 * 根据当前线型绘制连接线：
 * - Bezier: 贝塞尔曲线
 * - Straight: 直线
 * - Orthogonal: 直角折线
 */
void Connection::updatePath()
{
    if (DEBUG_CONNECTION_PATH) {
        qDebug() << "=== updatePath ===";
    }
    
    // 检查节点有效性
    if (!m_fromNode || !m_toNode) {
        if (DEBUG_CONNECTION_PATH) qDebug() << "节点无效，无法更新路径";
        return;
    }
    
    // 获取端点位置（使用指定的端口索引）
    QPointF startPos = m_fromNode->getOutputPortPos(m_fromPortIndex);
    QPointF endPos = m_toNode->getInputPortPos(m_toPortIndex);
    
    if (DEBUG_CONNECTION_PATH) {
        qDebug() << "源节点:" << m_fromNode->getName() 
                 << "端口:" << m_fromPortIndex << "位置:" << startPos;
        qDebug() << "目标节点:" << m_toNode->getName() 
                 << "端口:" << m_toPortIndex << "位置:" << endPos;
        qDebug() << "线型:" << lineTypeName(m_lineType);
    }
    
    // 创建路径
    QPainterPath path;
    path.moveTo(startPos);  // 移动到起点
    
    switch (m_lineType) {
    case Straight:
        // 直线：直接连接两点
        path.lineTo(endPos);
        break;
        
    case Orthogonal:
        {
            // 直角线：水平-垂直-水平的折线
            qreal dx = endPos.x() - startPos.x();
            qreal midX = startPos.x() + dx / 2;
            
            // 第一段：水平向右
            path.lineTo(midX, startPos.y());
            // 第二段：垂直移动
            path.lineTo(midX, endPos.y());
            // 第三段：水平到终点
            path.lineTo(endPos);
        }
        break;
        
    case Bezier:
    default:
        {
            // 贝塞尔曲线：平滑的曲线连接
            qreal dx = endPos.x() - startPos.x();
            
            // 计算控制点偏移量，考虑连接方向
            qreal ctrlOffset = qMax(qAbs(dx) * 0.4, 50.0);  // 至少50像素偏移
            
            // 第一个控制点：从起点出发，水平延伸
            QPointF ctrl1(startPos.x() + ctrlOffset, startPos.y());
            // 第二个控制点：从终点回退
            QPointF ctrl2(endPos.x() - ctrlOffset, endPos.y());
            
            if (DEBUG_CONNECTION_PATH) {
                qDebug() << "距离 dx:" << dx;
                qDebug() << "控制点1:" << ctrl1;
                qDebug() << "控制点2:" << ctrl2;
            }
            
            // 创建三次贝塞尔曲线
            path.cubicTo(ctrl1, ctrl2, endPos);
        }
        break;
    }
    
    setPath(path);
    
    if (DEBUG_CONNECTION_PATH) {
        qDebug() << "路径更新完成，起点:" << startPos << "终点:" << endPos;
    }
}

/**
 * @brief 打印连接线状态信息（仅在调试模式下有效）
 */
void Connection::printStatus() const
{
    if (DEBUG_CONNECTION_PATH) {
        qDebug() << "=== Connection状态 ===";
        qDebug() << "源节点:" << (m_fromNode ? m_fromNode->getName() : "nullptr")
                 << "端口:" << m_fromPortIndex;
        qDebug() << "目标节点:" << (m_toNode ? m_toNode->getName() : "nullptr")
                 << "端口:" << m_toPortIndex;
        if (m_fromNode && m_toNode) {
            qDebug() << "当前起点:" << m_fromNode->getOutputPortPos(m_fromPortIndex);
            qDebug() << "当前终点:" << m_toNode->getInputPortPos(m_toPortIndex);
        }
        qDebug() << "Z值:" << zValue();
        qDebug() << "可见:" << isVisible();
    }
}

/**
 * @brief 将连接线数据序列化为JSON对象
 * @return 包含连接信息的JSON对象
 */
QJsonObject Connection::toJson() const
{
    QJsonObject json;
    // 使用节点指针作为唯一标识符
    json["fromNode"] = QString::number(reinterpret_cast<quintptr>(m_fromNode));
    json["toNode"] = QString::number(reinterpret_cast<quintptr>(m_toNode));
    // 记录端口索引
    json["fromPortIndex"] = m_fromPortIndex;
    json["toPortIndex"] = m_toPortIndex;
    // 记录线型
    json["lineType"] = static_cast<int>(m_lineType);
    return json;
}
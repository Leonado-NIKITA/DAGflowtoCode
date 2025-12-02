/**
 * @file Node.cpp
 * @brief 节点类实现文件
 * @author 
 * @version 1.0.0
 * @date 2024
 */

#include "Node.h"
#include "Connection.h"
#include "NodeLibrary.h"
#include "NodeScene.h"
#include "UndoCommands.h"
#include "qgraphicssceneevent.h"
#include <QStyleOptionGraphicsItem>   // 图形项样式选项
#include <QJsonArray>                // JSON数组类
#include <QLineF>                    // 线段类（用于距离计算）
#include <QRadialGradient>           // 径向渐变类
#include <QPainter>                  // 绘制器类
#include <QTimer>                    // 定时器类

/**
 * @brief 构造函数
 * @param type 节点类型
 * @param name 节点名称
 * @param position 节点位置
 */
Node::Node(const QString &type, const QString &name, const QPointF &position)
    : m_type(type)                    // 初始化节点类型
    , m_name(name)                    // 初始化节点名称
    , m_dragStartPos(0, 0)            // 初始化拖拽起始位置
    , m_inputPortHighlighted(false)   // 初始化输入端口高亮状态
    , m_outputPortHighlighted(false)  // 初始化输出端口高亮状态
    , m_customColor(Qt::gray)         // 初始化自定义颜色
    , m_useCustomColor(false)         // 默认不使用自定义颜色
    , m_displayTypeName("")           // 初始化显示类型名称
    , m_inputPortCount(1)             // 默认1个输入端口
    , m_outputPortCount(1)            // 默认1个输出端口
    , m_width(DEFAULT_WIDTH)          // 初始化宽度
    , m_height(DEFAULT_HEIGHT)        // 初始化高度
    , m_resizing(false)               // 初始化调整大小状态
    , m_currentHandle(NoHandle)       // 初始化当前把手
{
    setPos(position);                 // 设置节点在场景中的位置
    
    // 从节点库获取模板信息
    NodeTemplate tmpl = NodeLibrary::instance()->getTemplate(type);
    if (tmpl.isValid()) {
        m_customColor = tmpl.getColor();
        m_useCustomColor = true;
        m_displayTypeName = tmpl.getDisplayName();
        m_inputPortCount = tmpl.getInputPortCount();
        m_outputPortCount = tmpl.getOutputPortCount();
    }
    
    // 设置图形项标志
    setFlag(QGraphicsItem::ItemIsMovable, true);           // 允许移动
    setFlag(QGraphicsItem::ItemIsSelectable, true);         // 允许选择
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true); // 发送几何变化信号
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);      // 启用设备坐标缓存以提高性能
    
    // 设置Z值，确保节点在连接线之上，端口在节点之上
    setZValue(0);  // 节点本身的Z值设为0
}

/**
 * @brief 析构函数，清理相关资源
 */
Node::~Node()
{
    // 注意：不在这里删除连接，由 NodeScene::deleteSelected 统一管理
    // 只需清空连接列表，避免悬空指针
    m_connections.clear();
}

/**
 * @brief 返回节点的边界矩形
 * @return 以节点中心为原点的边界矩形（包含端口区域和调整把手）
 */
QRectF Node::boundingRect() const
{
    // 端口圆心在节点边缘，所以只有半个圆在节点外
    const qreal portExtension = PORT_RADIUS + 2;  // 端口半径 + 边距
    const qreal handleExtension = HANDLE_SIZE / 2 + 2;  // 调整把手额外空间
    
    // 返回包含端口和调整把手的完整边界矩形
    qreal extraWidth = qMax(portExtension, handleExtension);
    qreal extraHeight = handleExtension;
    
    return QRectF(-m_width/2 - extraWidth, -m_height/2 - extraHeight, 
                  m_width + extraWidth * 2, m_height + extraHeight * 2);
}

/**
 * @brief 绘制节点的外观
 * @param painter 绘制器对象
 * @param option 样式选项（未使用）
 * @param widget 父控件（未使用）
 */
void Node::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    // 启用抗锯齿以提高绘制质量
    painter->setRenderHint(QPainter::Antialiasing);
    
    // 获取节点颜色：优先使用自定义颜色，否则使用内置颜色
    QColor color;
    if (m_useCustomColor) {
        color = m_customColor;
    } else {
        // 兼容内置节点类型
        if (m_type == "signal_source") color = QColor(81, 207, 102);     // 绿色 - 信号源
        else if (m_type == "filter") color = QColor(51, 154, 240);        // 蓝色 - 滤波器
        else if (m_type == "fft") color = QColor(204, 93, 232);           // 紫色 - FFT变换
        else if (m_type == "modulator") color = QColor(252, 196, 25);     // 黄色 - 调制器
        else if (m_type == "demodulator") color = QColor(255, 146, 43);   // 橙色 - 解调器
        else color = QColor(255, 107, 107);                               // 红色 - 输出
    }
    
    // 节点主体矩形（不包含端口区域）
    QRectF nodeRect(-m_width/2, -m_height/2, m_width, m_height);
    
    // 创建渐变填充效果
    QLinearGradient gradient(nodeRect.topLeft(), nodeRect.bottomRight());
    gradient.setColorAt(0, color.lighter(120));    // 起始颜色（较亮）
    gradient.setColorAt(1, color.darker(120));     // 结束颜色（较暗）
    
    // 绘制节点主体（圆角矩形）
    painter->setBrush(gradient);
    painter->setPen(isSelected() ? QPen(Qt::yellow, 3) : QPen(Qt::black, 2));  // 选中时显示黄色边框
    painter->drawRoundedRect(nodeRect, 10, 10);
    
    // 绘制节点名称文本
    painter->setPen(Qt::white);
    QFont font = painter->font();
    font.setBold(true);
    font.setPointSize(9);
    painter->setFont(font);
    painter->drawText(QRectF(-m_width/2, -m_height/2, m_width, 30), Qt::AlignCenter, m_name);
    
    // 绘制节点类型文本：优先使用显示名称
    font.setBold(false);
    font.setPointSize(8);
    painter->setFont(font);
    
    QString typeText;
    if (!m_displayTypeName.isEmpty()) {
        typeText = m_displayTypeName;
    } else {
        // 兼容内置节点类型
        if (m_type == "signal_source") typeText = "信号源";
        else if (m_type == "filter") typeText = "滤波器";
        else if (m_type == "fft") typeText = "FFT变换";
        else if (m_type == "modulator") typeText = "调制器";
        else if (m_type == "demodulator") typeText = "解调器";
        else typeText = m_type;  // 使用类型ID作为显示名称
    }
    
    painter->drawText(QRectF(-m_width/2, -m_height/2 + 25, m_width, 30), Qt::AlignCenter, typeText);
    
    // 绘制调整大小把手（仅在选中时显示）
    if (isSelected()) {
        drawResizeHandles(painter);
    }
    
    // 单独绘制端口以确保在节点框之上
    drawPorts(painter);
}


/**
 * @brief 设置节点类型
 * @param type 新的节点类型
 * 
 * 会从节点库获取新类型的颜色、显示名称和端口配置
 */
void Node::setType(const QString &type)
{
    if (m_type != type) {
        m_type = type;
        
        // 从节点库获取新类型的模板信息
        NodeTemplate tmpl = NodeLibrary::instance()->getTemplate(type);
        if (tmpl.isValid()) {
            m_customColor = tmpl.getColor();
            m_useCustomColor = true;
            m_displayTypeName = tmpl.getDisplayName();
            
            // 更新端口数量
            prepareGeometryChange();
            m_inputPortCount = tmpl.getInputPortCount();
            m_outputPortCount = tmpl.getOutputPortCount();
            
            // 更新所有相关连接线的位置
            for (Connection *conn : m_connections) {
                conn->updatePath();
            }
        }
        
        update();  // 触发重绘
    }
}

/**
 * @brief 设置节点名称
 * @param name 新的节点名称
 */
void Node::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        update();  // 触发重绘
    }
}

/**
 * @brief 设置节点参数
 * @param params 新的参数列表
 */
void Node::setParameters(const QStringList &params)
{
    m_parameters = params;
    update();  // 触发重绘
}

/**
 * @brief 设置自定义颜色
 * @param color 新的颜色
 */
void Node::setCustomColor(const QColor &color)
{
    m_customColor = color;
    m_useCustomColor = true;
    update();  // 触发重绘
}

/**
 * @brief 设置显示的类型名称
 * @param name 新的显示名称
 */
void Node::setDisplayTypeName(const QString &name)
{
    if (m_displayTypeName != name) {
        m_displayTypeName = name;
        update();  // 触发重绘
    }
}

/**
 * @brief 设置输入端口数量
 * @param count 新的端口数量
 */
void Node::setInputPortCount(int count)
{
    int newCount = qMax(0, count);
    if (m_inputPortCount != newCount) {
        prepareGeometryChange();  // 边界可能改变
        m_inputPortCount = newCount;
        
        // 更新所有相关连接线的位置
        for (Connection *conn : m_connections) {
            conn->updatePath();
        }
        
        update();  // 触发重绘
    }
}

/**
 * @brief 设置输出端口数量
 * @param count 新的端口数量
 */
void Node::setOutputPortCount(int count)
{
    int newCount = qMax(0, count);
    if (m_outputPortCount != newCount) {
        prepareGeometryChange();  // 边界可能改变
        m_outputPortCount = newCount;
        
        // 更新所有相关连接线的位置
        for (Connection *conn : m_connections) {
            conn->updatePath();
        }
        
        update();  // 触发重绘
    }
}

/**
 * @brief 添加与该节点相关的连接
 * @param connection 要添加的连接线指针
 */
void Node::addConnection(Connection *connection)
{
    m_connections.append(connection);
}

/**
 * @brief 移除与该节点相关的连接
 * @param connection 要移除的连接线指针
 */
void Node::removeConnection(Connection *connection)
{
    m_connections.removeAll(connection);
}

/**
 * @brief 计算端口在节点上的Y偏移量（场景坐标计算使用）
 * @param portIndex 端口索引
 * @param totalPorts 总端口数
 * @param nodeHeight 节点高度
 * @return Y方向偏移量
 */
static qreal calculatePortYOffset(int portIndex, int totalPorts, qreal nodeHeight)
{
    if (totalPorts <= 0) return 0;
    if (totalPorts == 1) return 0;  // 单端口居中
    
    // 多端口均匀分布，使用节点高度的80%作为分布区域
    const qreal usableHeight = nodeHeight * 0.8;
    const qreal startY = -usableHeight / 2;
    const qreal spacing = usableHeight / (totalPorts - 1);
    
    return startY + portIndex * spacing;
}

/**
 * @brief 获取输入端口在场景中的绝对位置（兼容单端口，返回第一个端口）
 * @return 第一个输入端口的场景坐标
 */
QPointF Node::getInputPortPos() const
{
    return getInputPortPos(0);
}

/**
 * @brief 获取输出端口在场景中的绝对位置（兼容单端口，返回第一个端口）
 * @return 第一个输出端口的场景坐标
 */
QPointF Node::getOutputPortPos() const
{
    return getOutputPortPos(0);
}

/**
 * @brief 获取指定索引的输入端口位置
 * @param portIndex 端口索引（从0开始）
 * @return 输入端口的场景坐标，如果没有输入端口则返回节点左侧中心位置
 * 
 * 注意：端口中心位于节点边缘，使端口圆的一半在节点内，一半在节点外
 */
QPointF Node::getInputPortPos(int portIndex) const
{
    // 如果没有输入端口，返回节点左侧中心位置
    if (m_inputPortCount <= 0) {
        return scenePos() + QPointF(-m_width/2, 0);
    }
    
    // 确保端口索引在有效范围内
    if (portIndex < 0 || portIndex >= m_inputPortCount) {
        portIndex = 0;
    }
    qreal yOffset = calculatePortYOffset(portIndex, m_inputPortCount, m_height);
    // 端口中心在节点边缘上
    return scenePos() + QPointF(-m_width/2, yOffset);
}

/**
 * @brief 获取指定索引的输出端口位置
 * @param portIndex 端口索引（从0开始）
 * @return 输出端口的场景坐标，如果没有输出端口则返回节点右侧中心位置
 * 
 * 注意：端口中心位于节点边缘，使端口圆的一半在节点内，一半在节点外
 */
QPointF Node::getOutputPortPos(int portIndex) const
{
    // 如果没有输出端口，返回节点右侧中心位置
    if (m_outputPortCount <= 0) {
        return scenePos() + QPointF(m_width/2, 0);
    }
    
    // 确保端口索引在有效范围内
    if (portIndex < 0 || portIndex >= m_outputPortCount) {
        portIndex = 0;
    }
    qreal yOffset = calculatePortYOffset(portIndex, m_outputPortCount, m_height);
    // 端口中心在节点边缘上
    return scenePos() + QPointF(m_width/2, yOffset);
}

/**
 * @brief 将节点数据序列化为JSON对象
 * @return 包含节点完整信息的JSON对象
 */
QJsonObject Node::toJson() const
{
    QJsonObject json;
    json["id"] = QString::number(reinterpret_cast<quintptr>(this));  // 使用对象指针作为唯一标识符
    json["type"] = m_type;                                            // 节点类型
    json["name"] = m_name;                                            // 节点名称
    json["x"] = x();                                                  // 节点X坐标
    json["y"] = y();                                                  // 节点Y坐标
    
    // 将参数列表转换为JSON数组
    QJsonArray paramsArray;
    for (const QString &param : m_parameters) {
        paramsArray.append(param);
    }
    json["parameters"] = paramsArray;                                 // 节点参数
    
    // 保存自定义颜色和显示名称
    if (m_useCustomColor) {
        json["customColor"] = m_customColor.name();
    }
    if (!m_displayTypeName.isEmpty()) {
        json["displayTypeName"] = m_displayTypeName;
    }
    
    // 保存端口数量
    json["inputPortCount"] = m_inputPortCount;
    json["outputPortCount"] = m_outputPortCount;
    
    // 保存节点尺寸
    json["width"] = m_width;
    json["height"] = m_height;
    
    return json;
}

/**
 * @brief 从JSON对象创建节点实例
 * @param json 包含节点信息的JSON对象
 * @return 新创建的节点指针
 */
Node* Node::fromJson(const QJsonObject &json)
{
    // 从JSON中提取基本信息
    QString type = json["type"].toString();      // 节点类型
    QString name = json["name"].toString();      // 节点名称
    qreal x, y;
    
    // 支持两种位置格式：直接x/y或position对象
    if (json.contains("position") && json["position"].isObject()) {
        QJsonObject posObj = json["position"].toObject();
        x = posObj["x"].toDouble();
        y = posObj["y"].toDouble();
    } else {
        x = json["x"].toDouble();
        y = json["y"].toDouble();
    }
    
    // 创建节点实例
    Node *node = new Node(type, name, QPointF(x, y));
    
    // 解析并设置节点参数
    QJsonArray paramsArray = json["parameters"].toArray();
    QStringList parameters;
    for (const QJsonValue &param : paramsArray) {
        parameters.append(param.toString());
    }
    node->setParameters(parameters);
    
    // 加载自定义颜色和显示名称
    if (json.contains("customColor")) {
        node->setCustomColor(QColor(json["customColor"].toString()));
    }
    if (json.contains("displayTypeName")) {
        node->setDisplayTypeName(json["displayTypeName"].toString());
    }
    
    // 加载端口数量
    if (json.contains("inputPortCount")) {
        node->setInputPortCount(json["inputPortCount"].toInt());
    }
    if (json.contains("outputPortCount")) {
        node->setOutputPortCount(json["outputPortCount"].toInt());
    }
    
    // 加载节点尺寸
    if (json.contains("width") && json.contains("height")) {
        node->setSize(json["width"].toDouble(), json["height"].toDouble());
    }
    
    return node;
}

/**
 * @brief 鼠标按下事件处理
 * @param event 鼠标事件对象
 * 
 * 记录起始位置，为后续拖拽或调整大小操作做准备
 */
void Node::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isSelected()) {
        // 检查是否点击了调整把手
        m_currentHandle = getResizeHandleAt(event->pos());
        if (m_currentHandle != NoHandle) {
            m_resizing = true;
            m_resizeStartPos = event->scenePos();
            m_resizeStartSize = QSizeF(m_width, m_height);
            event->accept();
            return;
        }
    }
    
    m_dragStartPos = pos();  // 记录拖拽起始位置
    QGraphicsItem::mousePressEvent(event);  // 调用基类处理
}

/**
 * @brief 鼠标移动事件处理
 * @param event 鼠标事件对象
 * 
 * 处理节点拖拽移动或调整大小，并同步更新所有相关连接线的位置
 */
void Node::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_resizing && m_currentHandle != NoHandle) {
        // 处理调整大小
        QPointF delta = event->scenePos() - m_resizeStartPos;
        qreal newWidth = m_resizeStartSize.width();
        qreal newHeight = m_resizeStartSize.height();
        
        switch (m_currentHandle) {
        case TopLeft:
            newWidth = m_resizeStartSize.width() - delta.x();
            newHeight = m_resizeStartSize.height() - delta.y();
            break;
        case TopRight:
            newWidth = m_resizeStartSize.width() + delta.x();
            newHeight = m_resizeStartSize.height() - delta.y();
            break;
        case BottomLeft:
            newWidth = m_resizeStartSize.width() - delta.x();
            newHeight = m_resizeStartSize.height() + delta.y();
            break;
        case BottomRight:
            newWidth = m_resizeStartSize.width() + delta.x();
            newHeight = m_resizeStartSize.height() + delta.y();
            break;
        case Left:
            newWidth = m_resizeStartSize.width() - delta.x();
            break;
        case Right:
            newWidth = m_resizeStartSize.width() + delta.x();
            break;
        case Top:
            newHeight = m_resizeStartSize.height() - delta.y();
            break;
        case Bottom:
            newHeight = m_resizeStartSize.height() + delta.y();
            break;
        default:
            break;
        }
        
        setSize(newWidth, newHeight);
        event->accept();
        return;
    }
    
    QGraphicsItem::mouseMoveEvent(event);  // 调用基类处理拖拽
    
    // 更新所有相关连接线的位置
    for (Connection *conn : m_connections) {
        conn->updatePath();
    }
}

/**
 * @brief 鼠标释放事件处理
 * @param event 鼠标事件对象
 * 
 * 完成拖拽或调整大小操作，并确保所有连接线位置正确
 */
void Node::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    // 用于防止多节点移动时重复创建命令
    static bool s_handlingMultiMove = false;
    
    if (m_resizing) {
        m_resizing = false;
        m_currentHandle = NoHandle;
    }
    
    QGraphicsItem::mouseReleaseEvent(event);  // 调用基类处理
    
    // 确保所有相关连接线位置正确
    for (Connection *conn : m_connections) {
        conn->updatePath();
    }
    
    // 如果位置发生了变化，创建移动撤销命令
    if (pos() != m_dragStartPos && !m_resizing && !s_handlingMultiMove) {
        NodeScene *nodeScene = qobject_cast<NodeScene*>(scene());
        if (nodeScene) {
            // 获取所有选中的节点
            QList<QGraphicsItem*> selectedItems = scene()->selectedItems();
            QList<Node*> selectedNodes;
            for (QGraphicsItem *item : selectedItems) {
                if (Node *node = dynamic_cast<Node*>(item)) {
                    selectedNodes.append(node);
                }
            }
            
            if (selectedNodes.size() > 1) {
                // 多节点移动：设置标志防止其他节点重复处理
                s_handlingMultiMove = true;
                
                // 计算位移量
                QPointF delta = pos() - m_dragStartPos;
                
                QList<QPointF> oldPositions;
                QList<QPointF> newPositions;
                
                for (Node *node : selectedNodes) {
                    QPointF newPos = node->pos();
                    QPointF oldPos = newPos - delta;
                    oldPositions.append(oldPos);
                    newPositions.append(newPos);
                    // 临时设置回原位置
                    node->setPos(oldPos);
                }
                
                MoveNodesCommand *cmd = new MoveNodesCommand(selectedNodes, oldPositions, newPositions);
                nodeScene->undoStack()->push(cmd);
                
                // 重置标志（使用 QTimer 延迟重置以确保所有节点的 mouseReleaseEvent 都已处理）
                QTimer::singleShot(0, []() { s_handlingMultiMove = false; });
            } else {
                // 单节点移动
                QPointF newPos = pos();
                setPos(m_dragStartPos);
                MoveNodeCommand *cmd = new MoveNodeCommand(this, m_dragStartPos, newPos);
                nodeScene->undoStack()->push(cmd);
            }
        }
    }
}

/**
 * @brief 图形项属性变化事件处理
 * @param change 变化类型
 * @param value 新值
 * @return 变化后的值
 */
QVariant Node::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange && scene()) {
        // 位置即将改变时，更新所有连接线
        // 这个调用在位置实际改变之前发生
        if (DEBUG_PORTS) {
            qDebug() << "节点" << m_name << "位置即将改变";
        }
    }
    else if (change == ItemPositionHasChanged && scene()) {
        // 位置已经改变后，立即更新所有连接线
        for (Connection *conn : m_connections) {
            conn->updatePath();
            if (DEBUG_PORTS) {
                conn->printStatus();  // 打印连接线状态
            }
        }
        if (DEBUG_PORTS) {
            qDebug() << "节点" << m_name << "位置已改变到" << pos() << "，更新了" << m_connections.size() << "条连接线";
        }
    }
    
    return QGraphicsItem::itemChange(change, value);
}

/**
 * @brief 检查点是否在任意输入端口的连接范围内
 * @param point 要检查的点（场景坐标）
 * @return 如果点在任意输入端口范围内返回true
 */
bool Node::isPointAtInputPort(const QPointF &point) const
{
    return getInputPortIndexAt(point) >= 0;
}

/**
 * @brief 检查点是否在任意输出端口的连接范围内
 * @param point 要检查的点（场景坐标）
 * @return 如果点在任意输出端口范围内返回true
 */
bool Node::isPointAtOutputPort(const QPointF &point) const
{
    return getOutputPortIndexAt(point) >= 0;
}

/**
 * @brief 获取鼠标位置对应的输入端口索引
 * @param point 要检查的点（场景坐标）
 * @return 端口索引，如果不在任何端口范围内或没有输入端口返回-1
 */
int Node::getInputPortIndexAt(const QPointF &point) const
{
    // 如果没有输入端口，直接返回-1
    if (m_inputPortCount <= 0) {
        return -1;
    }
    
    for (int i = 0; i < m_inputPortCount; ++i) {
        QPointF portPos = getInputPortPos(i);
        qreal distance = QLineF(point, portPos).length();
        if (distance <= PORT_CAPTURE_RADIUS) {
            if (DEBUG_PORTS) {
                qDebug() << "--- getInputPortIndexAt ---";
                qDebug() << "节点:" << m_name << "端口索引:" << i;
                qDebug() << "端口位置:" << portPos << "距离:" << distance;
            }
            return i;
        }
    }
    return -1;
}

/**
 * @brief 获取鼠标位置对应的输出端口索引
 * @param point 要检查的点（场景坐标）
 * @return 端口索引，如果不在任何端口范围内或没有输出端口返回-1
 */
int Node::getOutputPortIndexAt(const QPointF &point) const
{
    // 如果没有输出端口，直接返回-1
    if (m_outputPortCount <= 0) {
        return -1;
    }
    
    for (int i = 0; i < m_outputPortCount; ++i) {
        QPointF portPos = getOutputPortPos(i);
        qreal distance = QLineF(point, portPos).length();
        if (distance <= PORT_CAPTURE_RADIUS) {
            if (DEBUG_PORTS) {
                qDebug() << "--- getOutputPortIndexAt ---";
                qDebug() << "节点:" << m_name << "端口索引:" << i;
                qDebug() << "端口位置:" << portPos << "距离:" << distance;
            }
            return i;
        }
    }
    return -1;
}

/**
 * @brief 设置输入端口的高亮状态
 * @param highlighted 是否高亮
 */
void Node::setInputPortHighlighted(bool highlighted)
{
    if (m_inputPortHighlighted != highlighted) {
        m_inputPortHighlighted = highlighted;
        update();  // 重新绘制节点
    }
}

/**
 * @brief 设置输出端口的高亮状态
 * @param highlighted 是否高亮
 */
void Node::setOutputPortHighlighted(bool highlighted)
{
    if (m_outputPortHighlighted != highlighted) {
        m_outputPortHighlighted = highlighted;
        update();  // 重新绘制节点
    }
}

/**
 * @brief 绘制端口（单独绘制以确保在节点框之上）
 * @param painter 绘制器对象
 * 
 * 端口圆心位于节点边缘，使圆的一半在节点内，一半在节点外
 */
void Node::drawPorts(QPainter *painter)
{
    // 保存当前绘制状态
    painter->save();
    
    // 设置抗锯齿以确保圆形平滑
    painter->setRenderHint(QPainter::Antialiasing, true);
    
    // 设置合成模式为源覆盖，确保端口完全覆盖节点框
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    
    // 绘制所有输入端口（节点左侧）- 仅当端口数量大于0时
    // 端口中心在节点边缘上
    if (m_inputPortCount > 0) {
        for (int i = 0; i < m_inputPortCount; ++i) {
            qreal yOffset = calculatePortYOffset(i, m_inputPortCount, m_height);
            QPointF portCenter(-m_width/2, yOffset);  // 端口中心在节点边缘
            
            if (m_inputPortHighlighted) {
                // 高亮状态：更大的端口，绿色实心填充
                QRadialGradient gradient(portCenter, PORT_RADIUS + 2);
                gradient.setColorAt(0, QColor(100, 255, 100, 255));
                gradient.setColorAt(0.7, QColor(80, 230, 80, 255));
                gradient.setColorAt(1, QColor(30, 180, 30, 255));
                painter->setBrush(gradient);
                painter->setPen(QPen(QColor(20, 150, 20, 255), 2));
                painter->drawEllipse(portCenter, PORT_RADIUS + 2, PORT_RADIUS + 2);
            } else {
                // 普通状态：蓝色实心填充（输入端口）
                QRadialGradient gradient(portCenter, PORT_RADIUS);
                gradient.setColorAt(0, QColor(150, 200, 255, 255));
                gradient.setColorAt(0.8, QColor(100, 150, 220, 255));
                gradient.setColorAt(1, QColor(50, 100, 180, 255));
                painter->setBrush(gradient);
                painter->setPen(QPen(QColor(30, 80, 150, 255), 1.5));
                painter->drawEllipse(portCenter, PORT_RADIUS, PORT_RADIUS);
            }
            
            // 绘制端口索引标签（当有多个端口时）
            if (m_inputPortCount > 1) {
                painter->setPen(Qt::white);
                QFont font = painter->font();
                font.setPointSize(6);
                font.setBold(true);
                painter->setFont(font);
                painter->drawText(QRectF(portCenter.x() - PORT_RADIUS, portCenter.y() - PORT_RADIUS,
                                         PORT_RADIUS * 2, PORT_RADIUS * 2),
                                 Qt::AlignCenter, QString::number(i));
            }
        }
    }
    
    // 绘制所有输出端口（节点右侧）- 仅当端口数量大于0时
    // 端口中心在节点边缘上
    if (m_outputPortCount > 0) {
        for (int i = 0; i < m_outputPortCount; ++i) {
            qreal yOffset = calculatePortYOffset(i, m_outputPortCount, m_height);
            QPointF portCenter(m_width/2, yOffset);  // 端口中心在节点边缘
            
            if (m_outputPortHighlighted) {
                // 高亮状态：更大的端口，绿色实心填充
                QRadialGradient gradient(portCenter, PORT_RADIUS + 2);
                gradient.setColorAt(0, QColor(100, 255, 100, 255));
                gradient.setColorAt(0.7, QColor(80, 230, 80, 255));
                gradient.setColorAt(1, QColor(30, 180, 30, 255));
                painter->setBrush(gradient);
                painter->setPen(QPen(QColor(20, 150, 20, 255), 2));
                painter->drawEllipse(portCenter, PORT_RADIUS + 2, PORT_RADIUS + 2);
            } else {
                // 普通状态：橙色实心填充（输出端口）
                QRadialGradient gradient(portCenter, PORT_RADIUS);
                gradient.setColorAt(0, QColor(255, 220, 150, 255));
                gradient.setColorAt(0.8, QColor(255, 180, 100, 255));
                gradient.setColorAt(1, QColor(220, 140, 50, 255));
                painter->setBrush(gradient);
                painter->setPen(QPen(QColor(180, 100, 30, 255), 1.5));
                painter->drawEllipse(portCenter, PORT_RADIUS, PORT_RADIUS);
            }
            
            // 绘制端口索引标签（当有多个端口时）
            if (m_outputPortCount > 1) {
                painter->setPen(Qt::white);
                QFont font = painter->font();
                font.setPointSize(6);
                font.setBold(true);
                painter->setFont(font);
                painter->drawText(QRectF(portCenter.x() - PORT_RADIUS, portCenter.y() - PORT_RADIUS,
                                         PORT_RADIUS * 2, PORT_RADIUS * 2),
                                 Qt::AlignCenter, QString::number(i));
            }
        }
    }
    
    // 恢复绘制状态
    painter->restore();
}

/**
 * @brief 设置节点大小
 * @param width 新宽度
 * @param height 新高度
 */
void Node::setSize(qreal width, qreal height)
{
    prepareGeometryChange();
    m_width = qBound((qreal)MIN_WIDTH, width, (qreal)MAX_WIDTH);
    m_height = qBound((qreal)MIN_HEIGHT, height, (qreal)MAX_HEIGHT);
    
    // 更新所有连接线
    for (Connection *conn : m_connections) {
        conn->updatePath();
    }
    
    update();
}

/**
 * @brief 获取鼠标位置对应的调整把手
 * @param pos 本地坐标
 * @return 调整把手枚举
 */
Node::ResizeHandle Node::getResizeHandleAt(const QPointF &pos) const
{
    const qreal hs = HANDLE_SIZE;  // 把手大小
    const qreal hw = m_width / 2;
    const qreal hh = m_height / 2;
    
    // 四个角的把手
    if (QRectF(-hw - hs/2, -hh - hs/2, hs, hs).contains(pos)) return TopLeft;
    if (QRectF(hw - hs/2, -hh - hs/2, hs, hs).contains(pos)) return TopRight;
    if (QRectF(-hw - hs/2, hh - hs/2, hs, hs).contains(pos)) return BottomLeft;
    if (QRectF(hw - hs/2, hh - hs/2, hs, hs).contains(pos)) return BottomRight;
    
    // 四条边的把手
    if (QRectF(-hs/2, -hh - hs/2, hs, hs).contains(pos)) return Top;
    if (QRectF(-hs/2, hh - hs/2, hs, hs).contains(pos)) return Bottom;
    if (QRectF(-hw - hs/2, -hs/2, hs, hs).contains(pos)) return Left;
    if (QRectF(hw - hs/2, -hs/2, hs, hs).contains(pos)) return Right;
    
    return NoHandle;
}

/**
 * @brief 绘制调整把手
 * @param painter 绘制器
 */
void Node::drawResizeHandles(QPainter *painter)
{
    painter->save();
    
    const qreal hs = HANDLE_SIZE;
    const qreal hw = m_width / 2;
    const qreal hh = m_height / 2;
    
    // 设置把手样式
    painter->setBrush(QColor(255, 255, 255, 200));
    painter->setPen(QPen(QColor(100, 100, 100), 1));
    
    // 绘制四个角的把手
    painter->drawRect(QRectF(-hw - hs/2, -hh - hs/2, hs, hs));  // 左上
    painter->drawRect(QRectF(hw - hs/2, -hh - hs/2, hs, hs));   // 右上
    painter->drawRect(QRectF(-hw - hs/2, hh - hs/2, hs, hs));   // 左下
    painter->drawRect(QRectF(hw - hs/2, hh - hs/2, hs, hs));    // 右下
    
    // 绘制四条边中间的把手
    painter->drawRect(QRectF(-hs/2, -hh - hs/2, hs, hs));       // 上
    painter->drawRect(QRectF(-hs/2, hh - hs/2, hs, hs));        // 下
    painter->drawRect(QRectF(-hw - hs/2, -hs/2, hs, hs));       // 左
    painter->drawRect(QRectF(hw - hs/2, -hs/2, hs, hs));        // 右
    
    painter->restore();
}

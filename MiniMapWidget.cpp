/**
 * @file MiniMapWidget.cpp
 * @brief 导航小地图组件实现文件
 * @author
 * @version 1.0.0
 * @date 2024
 */

#include "MiniMapWidget.h"
#include "NodeView.h"
#include "qgraphicsitem.h"
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QPainter>
#include <QMouseEvent>
#include <QScrollBar>

/**
 * @brief 构造函数
 */
MiniMapWidget::MiniMapWidget(QWidget *parent)
    : QWidget(parent)
    , m_scene(nullptr)
    , m_mainView(nullptr)
    , m_dragging(false)
{
    setFixedSize(200, 150);
    setMouseTracking(true);
    
    // 设置背景样式
    setStyleSheet("background-color: rgba(40, 40, 50, 200); border: 1px solid #555; border-radius: 5px;");
}

/**
 * @brief 设置要显示的场景和主视图
 */
void MiniMapWidget::setSceneAndView(QGraphicsScene *scene, NodeView *mainView)
{
    m_scene = scene;
    m_mainView = mainView;
    
    if (m_mainView) {
        // 连接视图变化信号
        connect(m_mainView->horizontalScrollBar(), &QScrollBar::valueChanged,
                this, &MiniMapWidget::updateMiniMap);
        connect(m_mainView->verticalScrollBar(), &QScrollBar::valueChanged,
                this, &MiniMapWidget::updateMiniMap);
    }
    
    if (m_scene) {
        // 连接场景变化信号
        connect(m_scene, &QGraphicsScene::changed, this, &MiniMapWidget::updateMiniMap);
    }
    
    updateMiniMap();
}

/**
 * @brief 更新小地图显示
 */
void MiniMapWidget::updateMiniMap()
{
    update();
}

/**
 * @brief 获取缩放后的场景边界矩形
 */
QRectF MiniMapWidget::getSceneBounds() const
{
    if (!m_scene) return QRectF();
    
    // 获取场景中所有项的边界，如果为空则使用场景矩形
    QRectF bounds = m_scene->itemsBoundingRect();
    if (bounds.isEmpty()) {
        bounds = m_scene->sceneRect();
    }
    
    // 添加边距
    bounds.adjust(-100, -100, 100, 100);
    
    return bounds;
}

/**
 * @brief 将小地图坐标转换为场景坐标
 */
QPointF MiniMapWidget::widgetToScene(const QPointF &widgetPos) const
{
    QRectF sceneBounds = getSceneBounds();
    if (sceneBounds.isEmpty()) return QPointF();
    
    // 计算缩放比例
    qreal scaleX = sceneBounds.width() / (width() - 10);
    qreal scaleY = sceneBounds.height() / (height() - 10);
    qreal scale = qMax(scaleX, scaleY);
    
    // 计算偏移
    qreal offsetX = (width() - sceneBounds.width() / scale) / 2;
    qreal offsetY = (height() - sceneBounds.height() / scale) / 2;
    
    // 转换坐标
    qreal sceneX = (widgetPos.x() - offsetX) * scale + sceneBounds.left();
    qreal sceneY = (widgetPos.y() - offsetY) * scale + sceneBounds.top();
    
    return QPointF(sceneX, sceneY);
}

/**
 * @brief 将场景坐标转换为小地图坐标
 */
QPointF MiniMapWidget::sceneToWidget(const QPointF &scenePos) const
{
    QRectF sceneBounds = getSceneBounds();
    if (sceneBounds.isEmpty()) return QPointF();
    
    // 计算缩放比例
    qreal scaleX = sceneBounds.width() / (width() - 10);
    qreal scaleY = sceneBounds.height() / (height() - 10);
    qreal scale = qMax(scaleX, scaleY);
    
    // 计算偏移
    qreal offsetX = (width() - sceneBounds.width() / scale) / 2;
    qreal offsetY = (height() - sceneBounds.height() / scale) / 2;
    
    // 转换坐标
    qreal widgetX = (scenePos.x() - sceneBounds.left()) / scale + offsetX;
    qreal widgetY = (scenePos.y() - sceneBounds.top()) / scale + offsetY;
    
    return QPointF(widgetX, widgetY);
}

/**
 * @brief 绘制事件
 */
void MiniMapWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制背景
    painter.fillRect(rect(), QColor(40, 40, 50, 220));
    painter.setPen(QPen(QColor(80, 80, 90), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
    
    if (!m_scene || !m_mainView) return;
    
    QRectF sceneBounds = getSceneBounds();
    if (sceneBounds.isEmpty()) return;
    
    // 计算缩放比例
    qreal scaleX = sceneBounds.width() / (width() - 10);
    qreal scaleY = sceneBounds.height() / (height() - 10);
    qreal scale = qMax(scaleX, scaleY);
    
    // 计算偏移使内容居中
    qreal offsetX = (width() - sceneBounds.width() / scale) / 2;
    qreal offsetY = (height() - sceneBounds.height() / scale) / 2;
    
    // 设置变换
    painter.save();
    painter.translate(offsetX, offsetY);
    painter.scale(1.0 / scale, 1.0 / scale);
    painter.translate(-sceneBounds.left(), -sceneBounds.top());
    
    // 绘制网格背景
    painter.setPen(QPen(QColor(60, 60, 70, 100), scale));
    qreal gridSize = 100;
    qreal left = qFloor(sceneBounds.left() / gridSize) * gridSize;
    qreal top = qFloor(sceneBounds.top() / gridSize) * gridSize;
    for (qreal x = left; x < sceneBounds.right(); x += gridSize) {
        painter.drawLine(QPointF(x, sceneBounds.top()), QPointF(x, sceneBounds.bottom()));
    }
    for (qreal y = top; y < sceneBounds.bottom(); y += gridSize) {
        painter.drawLine(QPointF(sceneBounds.left(), y), QPointF(sceneBounds.right(), y));
    }
    
    // 绘制场景中的项（简化版本）
    // 先绘制连线（在节点下方）
    for (QGraphicsItem *item : m_scene->items()) {
        if (item->type() == QGraphicsItem::UserType + 2) {
            // Connection类型 - 绘制为细线
            QGraphicsPathItem *pathItem = dynamic_cast<QGraphicsPathItem*>(item);
            if (pathItem) {
                painter.setBrush(Qt::NoBrush);
                painter.setPen(QPen(QColor(255, 200, 50, 200), scale * 1.5));
                painter.drawPath(pathItem->path());
            }
        }
    }
    
    // 再绘制节点（在连线上方）
    for (QGraphicsItem *item : m_scene->items()) {
        if (item->type() == QGraphicsItem::UserType + 1) {
            // Node类型 - 绘制为小矩形（绿色）
            QRectF itemBounds = item->sceneBoundingRect();
            painter.setBrush(QColor(81, 207, 102, 200));
            painter.setPen(QPen(QColor(50, 150, 70), scale));
            painter.drawRoundedRect(itemBounds, 5 * scale, 5 * scale);
        } else if (item->type() == QGraphicsItem::UserType + 3) {
            // GroupNode类型 - 绘制为带边框的蓝色矩形
            QRectF itemBounds = item->sceneBoundingRect();
            // 绘制外边框（表示组）
            painter.setBrush(QColor(100, 149, 237, 180));  // 康乃馨蓝
            painter.setPen(QPen(QColor(70, 100, 180), scale * 2));
            painter.drawRoundedRect(itemBounds, 8 * scale, 8 * scale);
            // 绘制内部虚线边框
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor(255, 255, 255, 150), scale, Qt::DashLine));
            painter.drawRoundedRect(itemBounds.adjusted(3*scale, 3*scale, -3*scale, -3*scale), 
                                   5 * scale, 5 * scale);
        }
    }
    
    painter.restore();
    
    // 绘制当前视口区域
    QRectF viewportRect = m_mainView->mapToScene(m_mainView->viewport()->rect()).boundingRect();
    QPointF topLeft = sceneToWidget(viewportRect.topLeft());
    QPointF bottomRight = sceneToWidget(viewportRect.bottomRight());
    QRectF viewportInMiniMap(topLeft, bottomRight);
    
    // 绘制视口边框
    painter.setPen(QPen(QColor(255, 100, 100), 2));
    painter.setBrush(QColor(255, 100, 100, 30));
    painter.drawRect(viewportInMiniMap);
    
    // 绘制标题
    painter.setPen(QColor(200, 200, 200));
    QFont font = painter.font();
    font.setPointSize(8);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(5, 12, "导航");
}

/**
 * @brief 鼠标按下事件
 */
void MiniMapWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_mainView) {
        m_dragging = true;
        m_dragStartPos = event->pos();
        
        // 将视图中心移动到点击位置
        QPointF scenePos = widgetToScene(event->pos());
        m_mainView->centerOn(scenePos);
        
        event->accept();
    }
}

/**
 * @brief 鼠标移动事件
 */
void MiniMapWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && m_mainView) {
        // 将视图中心移动到当前位置
        QPointF scenePos = widgetToScene(event->pos());
        m_mainView->centerOn(scenePos);
        
        event->accept();
    }
}

/**
 * @brief 鼠标释放事件
 */
void MiniMapWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}


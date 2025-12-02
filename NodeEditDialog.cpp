/**
 * @file NodeEditDialog.cpp
 * @brief 节点编辑对话框类实现文件
 * @author
 * @version 1.0.0
 * @date 2024
 */

#include "NodeEditDialog.h"
#include "NodeLibrary.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QColorDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QRegularExpression>

/**
 * @brief 构造函数
 */
NodeEditDialog::NodeEditDialog(Mode mode, QWidget *parent)
    : QDialog(parent)
    , m_mode(mode)
    , m_selectedColor(Qt::gray)
{
    setupUI();

    if (mode == CreateMode) {
        setWindowTitle("创建自定义节点");
    } else {
        setWindowTitle("编辑节点模板");
    }
}

/**
 * @brief 设置用户界面
 */
void NodeEditDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 基本信息组
    QGroupBox *basicGroup = new QGroupBox("基本信息");
    QFormLayout *basicLayout = new QFormLayout();

    m_typeIdEdit = new QLineEdit();
    m_typeIdEdit->setPlaceholderText("例如: custom_processor");
    if (m_mode == EditMode) {
        m_typeIdEdit->setReadOnly(true);  // 编辑模式下不允许修改类型ID
    }

    m_displayNameEdit = new QLineEdit();
    m_displayNameEdit->setPlaceholderText("例如: 自定义处理器");

    m_categoryCombo = new QComboBox();
    m_categoryCombo->setEditable(true);
    // 添加现有分类
    QStringList categories = NodeLibrary::instance()->getCategories();
    m_categoryCombo->addItems(categories);
    if (!categories.contains("自定义")) {
        m_categoryCombo->addItem("自定义");
    }

    basicLayout->addRow("类型标识:", m_typeIdEdit);
    basicLayout->addRow("显示名称:", m_displayNameEdit);
    basicLayout->addRow("所属分类:", m_categoryCombo);

    basicGroup->setLayout(basicLayout);
    mainLayout->addWidget(basicGroup);

    // 外观设置组
    QGroupBox *appearanceGroup = new QGroupBox("外观设置");
    QFormLayout *appearanceLayout = new QFormLayout();

    QHBoxLayout *colorLayout = new QHBoxLayout();
    m_colorButton = new QPushButton("选择颜色...");
    m_colorPreview = new QLabel();
    m_colorPreview->setFixedSize(50, 25);
    m_colorPreview->setAutoFillBackground(true);
    updateColorPreview();

    colorLayout->addWidget(m_colorButton);
    colorLayout->addWidget(m_colorPreview);
    colorLayout->addStretch();

    connect(m_colorButton, &QPushButton::clicked, this, &NodeEditDialog::onSelectColor);

    appearanceLayout->addRow("节点颜色:", colorLayout);

    appearanceGroup->setLayout(appearanceLayout);
    mainLayout->addWidget(appearanceGroup);

    // 端口配置组
    QGroupBox *portGroup = new QGroupBox("端口配置");
    QFormLayout *portLayout = new QFormLayout();

    m_inputPortSpin = new QSpinBox();
    m_inputPortSpin->setRange(0, 10);
    m_inputPortSpin->setValue(1);

    m_outputPortSpin = new QSpinBox();
    m_outputPortSpin->setRange(0, 10);
    m_outputPortSpin->setValue(1);

    portLayout->addRow("输入端口数量:", m_inputPortSpin);
    portLayout->addRow("输出端口数量:", m_outputPortSpin);

    portGroup->setLayout(portLayout);
    mainLayout->addWidget(portGroup);

    // 描述和参数组
    QGroupBox *descGroup = new QGroupBox("描述和参数");
    QFormLayout *descLayout = new QFormLayout();

    m_descriptionEdit = new QTextEdit();
    m_descriptionEdit->setMaximumHeight(80);
    m_descriptionEdit->setPlaceholderText("节点功能描述...");

    m_paramsEdit = new QLineEdit();
    m_paramsEdit->setPlaceholderText("默认参数，用逗号分隔");

    descLayout->addRow("描述:", m_descriptionEdit);
    descLayout->addRow("默认参数:", m_paramsEdit);

    descGroup->setLayout(descLayout);
    mainLayout->addWidget(descGroup);

    // 按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &NodeEditDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    setMinimumWidth(400);
}

/**
 * @brief 设置要编辑的节点模板
 */
void NodeEditDialog::setTemplate(const NodeTemplate &tmpl)
{
    m_typeIdEdit->setText(tmpl.getTypeId());
    m_displayNameEdit->setText(tmpl.getDisplayName());
    m_categoryCombo->setCurrentText(tmpl.getCategory());
    m_selectedColor = tmpl.getColor();
    updateColorPreview();
    m_inputPortSpin->setValue(tmpl.getInputPortCount());
    m_outputPortSpin->setValue(tmpl.getOutputPortCount());
    m_descriptionEdit->setPlainText(tmpl.getDescription());
    m_paramsEdit->setText(tmpl.getDefaultParameters().join(", "));
}

/**
 * @brief 获取编辑后的节点模板
 */
NodeTemplate NodeEditDialog::getTemplate() const
{
    NodeTemplate tmpl;
    tmpl.setTypeId(m_typeIdEdit->text().trimmed());
    tmpl.setDisplayName(m_displayNameEdit->text().trimmed());
    tmpl.setCategory(m_categoryCombo->currentText().trimmed());
    tmpl.setColor(m_selectedColor);
    tmpl.setInputPortCount(m_inputPortSpin->value());
    tmpl.setOutputPortCount(m_outputPortSpin->value());
    tmpl.setDescription(m_descriptionEdit->toPlainText().trimmed());

    QString paramsText = m_paramsEdit->text().trimmed();
    if (!paramsText.isEmpty()) {
        tmpl.setDefaultParameters(paramsText.split(',', Qt::SkipEmptyParts));
    }

    tmpl.setBuiltIn(false);  // 用户创建的节点不是内置节点

    return tmpl;
}

/**
 * @brief 选择颜色按钮点击事件
 */
void NodeEditDialog::onSelectColor()
{
    QColor color = QColorDialog::getColor(m_selectedColor, this, "选择节点颜色");
    if (color.isValid()) {
        m_selectedColor = color;
        updateColorPreview();
    }
}

/**
 * @brief 更新颜色预览
 */
void NodeEditDialog::updateColorPreview()
{
    QPalette palette = m_colorPreview->palette();
    palette.setColor(QPalette::Window, m_selectedColor);
    m_colorPreview->setPalette(palette);
}

/**
 * @brief 验证输入并接受对话框
 */
void NodeEditDialog::onAccept()
{
    // 验证类型ID
    QString typeId = m_typeIdEdit->text().trimmed();
    if (typeId.isEmpty()) {
        QMessageBox::warning(this, "验证错误", "类型标识不能为空！");
        m_typeIdEdit->setFocus();
        return;
    }

    // 检查类型ID是否包含无效字符
    QRegularExpression rx("^[a-zA-Z_][a-zA-Z0-9_]*$");
    if (!rx.match(typeId).hasMatch()) {
        QMessageBox::warning(this, "验证错误",
            "类型标识只能包含字母、数字和下划线，且不能以数字开头！");
        m_typeIdEdit->setFocus();
        return;
    }

    // 创建模式下检查是否已存在
    if (m_mode == CreateMode && NodeLibrary::instance()->hasTemplate(typeId)) {
        QMessageBox::warning(this, "验证错误",
            QString("类型标识 '%1' 已存在！").arg(typeId));
        m_typeIdEdit->setFocus();
        return;
    }

    // 验证显示名称
    QString displayName = m_displayNameEdit->text().trimmed();
    if (displayName.isEmpty()) {
        QMessageBox::warning(this, "验证错误", "显示名称不能为空！");
        m_displayNameEdit->setFocus();
        return;
    }

    accept();
}


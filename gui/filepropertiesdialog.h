#ifndef FILEPROPERTIESDIALOG_H
#define FILEPROPERTIESDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>

class FilePropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FilePropertiesDialog(const QString &fileName, QWidget *parent = nullptr);

private:
    void setupUI();
    void applyStyles();
    QWidget* makePropRow(const QString &label, const QString &value, const QString &valueColor = "#d0d0e0");

    QString m_fileName;
};

#endif

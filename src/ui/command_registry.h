#pragma once

#include <QList>
#include <QObject>
#include <QString>

class QAction;
class QMenu;

class CommandRegistry final : public QObject {
    Q_OBJECT

public:
    explicit CommandRegistry(QObject* parent = nullptr);

    QAction* add(const QString& id, const QString& text, bool enabled = true);
    QAction* get(const QString& id) const;
    void add_to_menu(QMenu* menu, const QString& id) const;

private:
    QList<QAction*> actions_;
};

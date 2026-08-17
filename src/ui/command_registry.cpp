#include "ui/command_registry.h"

#include <QAction>
#include <QMenu>

CommandRegistry::CommandRegistry(QObject* parent)
    : QObject(parent)
{
}

QAction* CommandRegistry::add(
    const QString& id,
    const QString& text,
    bool enabled)
{
    auto* action = new QAction(text, this);
    action->setObjectName(id);
    action->setEnabled(enabled);
    actions_.append(action);
    return action;
}

QAction* CommandRegistry::get(const QString& id) const
{
    for (QAction* action : actions_) {
        if (action->objectName() == id) {
            return action;
        }
    }
    return nullptr;
}

void CommandRegistry::add_to_menu(QMenu* menu, const QString& id) const
{
    if (QAction* action = get(id)) {
        menu->addAction(action);
    }
}

#ifndef OBSERVER_INTERFACES_H
#define OBSERVER_INTERFACES_H
#pragma once

#include <vector>
#include <algorithm>

// Forward-declare the Subject so the Observer knows about it.
class ISubject;

/**
 * @class IObserver
 * @brief Represents an object that listens for notifications from an ISubject.
 */
class IObserver {
public:
    virtual ~IObserver() = default;

    /**
     * @brief The function called by an ISubject when it notifies its observers.
     * @param subject A pointer to the subject that triggered the notification.
     */
    virtual void onNotify(const ISubject* subject) = 0;
};


/**
 * @class ISubject
 * @brief Represents an object that can be observed. It maintains a list of
 * IObservers and notifies them of changes.
 */
class ISubject {
public:
    virtual ~ISubject() = default;

    /**
     * @brief Subscribes an observer to receive notifications.
     */
    void addObserver(IObserver* observer) {
        if (observer) {
            m_observers.push_back(observer);
        }
    }

    /**
     * @brief Unsubscribes an observer from receiving notifications.
     */
    void removeObserver(IObserver* observer) {
        m_observers.erase(
            std::remove(m_observers.begin(), m_observers.end(), observer),
            m_observers.end()
        );
    }

protected:
    /**
     * @brief Notifies all registered observers by calling their onNotify method.
     */
    void notify() {
        for (IObserver* observer : m_observers) {
            observer->onNotify(this);
        }
    }

private:
    std::vector<IObserver*> m_observers;
};

#endif // OBSERVER_INTERFACES_H

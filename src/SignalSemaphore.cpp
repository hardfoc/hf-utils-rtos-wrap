/**
 *
 * @todo Add @copyright line once project copyright wording is finalised.
 */

#include "SignalSemaphore.h"
#include <cstring>

/**
 * @par Example
 * @code
 * #include "SignalSemaphore.h"
 *
 * int main() {
 *     SignalSemaphore semaphore("MySemaphore");
 *
 *     // Signal the semaphore
 *     semaphore.Signal();
 *
 *     // Wait until the semaphore is signaled
 *     if (semaphore.WaitUntilSignalled(1000)) {
 *         // Semaphore was signaled
 *     } else {
 *         // Timeout occurred
 *     }
 *
 *     return 0;
 * }
 * @endcode
 */

/**
 * @brief Constructor for SignalSemaphore.
 * @param baseName Base name of the semaphore.
 * @param nameExtension Optional name extension for uniqueness.
 */
SignalSemaphore::SignalSemaphore(const char* baseName, const char* nameExtension) noexcept :
    semaphore{},
    name{},
    initialized(false)
{
    if (nameExtension != nullptr)
    {
        snprintf(name, MaxNameLength + 1, "%s-%s", baseName, nameExtension);
    }
    else
    {
        strncpy(name, baseName, MaxNameLength);
        name[MaxNameLength] = '\0';
    }
}

/**
 * @brief Destructor for SignalSemaphore.
 */
SignalSemaphore::~SignalSemaphore()
{
    if (initialized)
    {
        os_semaphore_delete_ex(&semaphore);
    }
}

/**
 * @brief Waits until the semaphore is signaled.
 * @param msecToWait Maximum time to wait in milliseconds.
 * @return True if the semaphore was successfully acquired, false otherwise.
 */
bool SignalSemaphore::WaitUntilSignalled(uint32_t msecToWait) noexcept
{
    if (EnsureInitialized())
    {
        return (os_semaphore_get_ex(&semaphore, msecToWait));
    }
    return false;
}

/**
 * @brief Signals the semaphore.
 * @return True if the semaphore was successfully signaled, false otherwise.
 */
bool SignalSemaphore::Signal() noexcept
{
    if (EnsureInitialized())
    {
        os_semaphore_put_ex(&semaphore);
        return true;
    }
    return false;
}

/**
 * @brief Checks if the semaphore is signaled.
 * @return True if the semaphore is signaled, false otherwise.
 */
bool SignalSemaphore::IsSignalled() noexcept
{
    if (EnsureInitialized())
    {
        OS_Ulong currentValue = os_semaphore_get_count_ex(&semaphore);
        return (currentValue > 0);
    }
    return false;
}

/**
 * @brief Checks if the semaphore is initialized.
 * @return True if the semaphore is initialized, false otherwise.
 */
bool SignalSemaphore::IsInitialized() const noexcept
{
    return initialized;
}

/**
 * @brief Gets the name of the semaphore.
 * @return The name of the semaphore.
 */
const char* SignalSemaphore::GetName() const noexcept
{
    return name;
}

/**
 * @brief Ensures the semaphore is initialized.
 * @return True if the semaphore is initialized, false otherwise.
 */
bool SignalSemaphore::EnsureInitialized() noexcept
{
    if (!initialized)
    {
        initialized = os_semaphore_create_ex(&semaphore, name, 0);
    }
    return initialized;
}

/**
 * @par Example
 * @code
 * #include "SignalSemaphore.h"
 *
 * int main() {
 *     SignalSemaphore semaphore("MySemaphore");
 *
 *     // Signal the semaphore
 *     semaphore.Signal();
 *
 *     // Wait until the semaphore is signaled
 *     if (semaphore.WaitUntilSignalled(1000)) {
 *         // Semaphore was signaled
 *     } else {
 *         // Timeout occurred
 *     }
 *
 *     return 0;
 * }
 * @endcode
 */

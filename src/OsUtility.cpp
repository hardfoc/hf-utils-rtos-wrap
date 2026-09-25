/**
  *
  *  Contains the declaration and definition of the utility functions related to
  *     the FreeRTOS RTOS.
  *
  *   Note:  These functions are not thread or interrupt-safe and should be called
  *          called with appropriate guards if used within an ISR or shared between tasks.
 * @todo Add @copyright line once project copyright wording is finalised.
  */
#include <cmath>
#include "OsAbstraction.h"
#include "OsUtility.h"
#if defined(HF_RTOS_FREERTOS)
#include "FreeRTOSUtils.h"
#endif

//==============================================================//
// GLOBALS
//==============================================================//
// os_critical_mux is defined in CriticalSection.cpp


//==============================================================//
// FUNCTIONS
//==============================================================//

void os_delay_msec( uint16_t msec )
{
	uint32_t ticks = os_convert_msec_to_delay_ticks( msec );

	uint32_t result = os_thread_sleep (ticks);
	if( result != OS_SUCCESS )
	{
	}
}

void os_delay_time(uint32_t timeBetweenSamples, time_unit_t timeUnit)
{
    if(timeBetweenSamples == 0){
        return;
    }

    switch(timeUnit)
    {
		case(TIME_UNIT_US):
		{
			os_delay_msec( static_cast<uint16_t>(timeBetweenSamples/1000000U ));
			break;
		}
        case(TIME_UNIT_MS):
        {
            os_delay_msec((uint16_t)timeBetweenSamples);
            break;
        }
        case(TIME_UNIT_S):
        {
            os_delay_msec(static_cast<uint16_t>(timeBetweenSamples*1000));
            break;
        }
        default: break;
    }
}

uint32_t os_get_elapsed_time_msec() {
	float elapsedTime = static_cast<float>(os_time_get()) * 1000.0F / osTickRateHz;
    return static_cast<uint32_t>(std::ceil(elapsedTime));
}

uint32_t os_get_elapsed_processor_cycle_count(uint32_t startCycleCount, time_unit_t unit )
{
    static const float time_unit_us_divider = (1000000.0);
    static const float time_unit_ms_divider = (1000.0);
	static const float time_unit_s_divider = (1.0);
	float divider;

    switch(unit){
    	case(TIME_UNIT_US):
		{
    		divider = time_unit_us_divider;
    		break;
		}
    	case(TIME_UNIT_MS):
		{
    		divider = time_unit_ms_divider;
			break;
		}
    	case(TIME_UNIT_S):
		{
    		divider = time_unit_s_divider;
			break;
		}
    	default:
		{
    		divider = time_unit_ms_divider;
			break;
		}
    }

    uint32_t current_ticks = static_cast<uint32_t>(os_time_get());
    uint32_t elapsed_ticks;
    
    if(startCycleCount > current_ticks)
    {
        elapsed_ticks = ~(startCycleCount - current_ticks);
    }
    else
    {
        elapsed_ticks = current_ticks - startCycleCount;
    }

    // Convert ticks to the requested time unit
    // Note: This is an approximation since we're using ticks instead of actual cycles
    return (uint32_t)(elapsed_ticks * (1000.0 / osTickRateHz) / (1000.0 / divider));
}

uint32_t os_get_processor_cycle_count(){
    return static_cast<uint32_t>(os_time_get());
}

//constexpr uint32_t os_convert_msec_to_delay_ticks( uint32_t milliseconds )
//{
//
//	//Calculate the delay in terms of the threadx tick rate */
//	return milliseconds * osTickRateHz / 1000U;
//}
//
//constexpr uint32_t os_convert_delay_ticks_to_msec(uint32_t delayTicks)
//{
//    // Calculate the delay in terms of milliseconds
//    return (delayTicks * 1000U) / osTickRateHz;
//}
//
//constexpr uint32_t os_convert_hz_to_delay_ticks( uint32_t frequency )
//{
//	// Calculate the delay in terms of the threadx tick rate
//	return osTickRateHz/frequency;
//}

//============================================================================================//
// MUTEX
//============================================================================================//

/**
 * @brief Creates a mutex with the specified parameters.
 *
 * @param mutex - The OS mutex to create
 * @param name - The name fo rthe mutex
 * @param inherit - The inherit property for the mutex
 */
bool os_mutex_create_ex(OS_Mutex& mutex, const char* mutexName, OS_Uint inherit, [[maybe_unused]] bool suppressVerbose) noexcept
{
	if( mutexName != nullptr )
	{
                OS_Uint status = os_mutex_create(&mutex, const_cast<char*>(mutexName), inherit);
		if (status == OS_SUCCESS)
		{
			return true;
		}
		else
		{
		}
	}
	else
	{
	}

	// TODO: Error Handling
	return false;
}

bool os_mutex_create_p(OS_Mutex* mutex, const char* mutexName, OS_Uint inherit, [[maybe_unused]] bool suppressVerbose) noexcept
{
	if( mutexName != nullptr )
	{
                OS_Uint status = os_mutex_create(mutex, const_cast<char*>(mutexName), inherit);
		if (status == OS_SUCCESS)
		{
			return true;
		}
		else
		{
		}
	}
	else
	{
	}

	// TODO: Error Handling
	return false;
}

/**
 * @brief Acquire (wait for) a TX mutex.
 * @param mutex_ptr Pointer to the mutex control block.
 * @param wait_option Specifies the maximum time to wait for the mutex.
 * @param suppressVerbose Flag to suppress verbose output.
 * @return true if the mutex was successfully acquired, false otherwise.
 */
bool os_mutex_get_ex(OS_Mutex& mutex, OS_Ulong wait_option, [[maybe_unused]] bool suppressVerbose) noexcept
{
    OS_Uint err = os_mutex_get(&mutex, wait_option);

    if (err == OS_SUCCESS)
    {
        return true;
    }
    else
    {
        // TBD: Log appropriate error here
        return false;
    }
}

/**
 * @brief Acquire (wait for) a TX mutex.
 * @param mutex_ptr Pointer to the mutex control block.
 * @param wait_option Specifies the maximum time to wait for the mutex.
 * @param suppressVerbose Flag to suppress verbose output.
 * @return true if the mutex was successfully acquired, false otherwise.
 */
bool os_mutex_get_p(OS_Mutex* mutex, OS_Ulong wait_option, [[maybe_unused]] bool suppressVerbose) noexcept
{
    OS_Uint err = os_mutex_get(mutex, wait_option);

    if (err == OS_SUCCESS)
    {
        return true;
    }
    else
    {
        // TBD: Log appropriate error here
        return false;
    }
}

/**
 * @brief Release a TX mutex.
 * @param mutex_ptr Pointer to the mutex control block.
 * @param suppressVerbose Flag to suppress verbose output.
 * @return true if the mutex was successfully released, false otherwise.
 */
bool os_mutex_put_ex(OS_Mutex& mutex, [[maybe_unused]] bool suppressVerbose) noexcept
{
    OS_Uint err = os_mutex_put(&mutex);

    if (err == OS_SUCCESS)
    {
        return true;
    }
    else
    {
        // TBD: Log appropriate error here
        return false;
    }
}

/**
 * @brief Release a TX mutex.
 * @param mutex_ptr Pointer to the mutex control block.
 * @param suppressVerbose Flag to suppress verbose output.
 * @return true if the mutex was successfully released, false otherwise.
 */
bool os_mutex_put_p(OS_Mutex* mutex, [[maybe_unused]] bool suppressVerbose) noexcept
{
    OS_Uint err = os_mutex_put(mutex);

    if (err == OS_SUCCESS)
    {
        return true;
    }
    else
    {
        // TBD: Log appropriate error here
        return false;
    }
}

/**
 * @brief Deletes the specified mutex and logs any error.
 *
 * @param mutex - The OS mutex to delete
 **/
bool os_mutex_delete_ex(OS_Mutex& mutex, [[maybe_unused]] bool suppressVerbose) noexcept
{
        OS_Uint status = os_mutex_delete( &mutex );
	if( status != OS_SUCCESS)
	{
		(void)suppressVerbose;
		return false;
	}

	return true;
}

/**
 * @brief Deletes the specified mutex and logs any error.
 *
 * @param mutex - The OS mutex to delete
 **/
bool os_mutex_delete_p(OS_Mutex* mutex, [[maybe_unused]] bool suppressVerbose) noexcept
{
        OS_Uint status = os_mutex_delete( mutex );
	if( status != OS_SUCCESS)
	{
		if( mutex )
		{
		}
		else
		{
		}
		return false;
	}

	return true;
}

//============================================================================================//
// THREAD
//============================================================================================//

//================//
// THREAD COUNTS
//================//

/// Provide a default definition if not supplied by higher-level code (weak symbol).
__attribute__((weak)) uint32_t g_ssp_common_thread_count = 0;

/**
 * @brief Function to Create TX thread.
 * @param txThread Pointer to the OS thread control block.
 * @param name Name of the thread.
 * @param entry_function Pointer to the thread's entry function.
 * @param entry_input Input value for the thread's entry function.
 * @param stack Pointer to the thread's stack.
 * @param stackSizeBytes Size of the thread's stack in bytes.
 * @param priority Priority of the thread.
 * @param preempt_threshold Preemption threshold of the thread.
 * @param timeSliceAllowed Maximum length of time (in ticks) that the thread can execute before OS will reschedule.
 * @param auto_start OS_AUTO_START if the thread should start immediately, or OS_DONT_START if it should not start until explicitly resumed.
 * @return true if the thread was successfully created, false otherwise.
 */
bool os_thread_create_ex(OS_Thread* txThread, const char* name,
                    void (*entry_function)(OS_Ulong id), OS_Ulong entry_input,
                    uint8_t* stack, OS_Ulong stackSizeBytes, OS_Uint priority,
                    OS_Uint preempt_threshold, OS_Ulong timeSliceAllowed,
                    OS_Uint auto_start, [[maybe_unused]] bool suppressVerbose ) noexcept
{
    /// Create the OS Thread
    OS_Uint err = os_thread_create(txThread, const_cast<char*>(name), entry_function, entry_input, stack, stackSizeBytes, priority, preempt_threshold, timeSliceAllowed, auto_start);

    if (err == OS_SUCCESS)
    {
        /// Add to the overall system thread count
        ++g_ssp_common_thread_count;

        return true;
    }
    else
    {
    	 os_delay_msec( 5 );
        /// TODO: log appropriate error here
        return false;
    }
}

bool os_thread_create_ex_pinned(OS_Thread* txThread, const char* name,
                    void (*entry_function)(OS_Ulong id), OS_Ulong entry_input,
                    uint8_t* stack, OS_Ulong stackSizeBytes, OS_Uint priority,
                    OS_Uint preempt_threshold, OS_Ulong timeSliceAllowed,
                    OS_Uint auto_start, int core_id, [[maybe_unused]] bool suppressVerbose ) noexcept
{
    OS_Uint err = os_thread_create_pinned(txThread, const_cast<char*>(name),
                                          entry_function, entry_input, stack,
                                          stackSizeBytes, priority,
                                          preempt_threshold, timeSliceAllowed,
                                          auto_start, core_id);
    if (err == OS_SUCCESS)
    {
        ++g_ssp_common_thread_count;
        return true;
    }
    os_delay_msec( 5 );
    return false;
}

/**
 * @brief Function to Delete TX thread.
 * @param txThread Pointer to the OS thread control block to be deleted.
 * @return true if the thread was successfully deleted, false otherwise.
 */
bool os_thread_delete_ex(OS_Thread *txThread, [[maybe_unused]] bool suppressVerbose) noexcept
{
    if (!os_thread_is_live(txThread)) {
        return true;
    }
    (void)suppressVerbose;
    os_thread_delete(txThread);
    os_thread_clear(txThread);
    if (g_ssp_common_thread_count > 0) {
        --g_ssp_common_thread_count;
    }
    return true;
}

bool os_thread_resume_ex( OS_Thread* thread, [[maybe_unused]] bool suppressVerbose )
{
	bool success = false;
	if( thread != nullptr )
	{
		if( os_thread_is_live(thread) )
		{

			uint32_t result = os_thread_resume(thread);

			os_delay_msec( 10 ); // Give the thread a chance to start


			if (result != OS_SUCCESS)
			{
			}
			else
			{
				success = true;
			}
		}
		else
		{
		}
	}
	else
	{
	}
	return success;
}

bool os_thread_resume_if_suspended(OS_Thread *thread, [[maybe_unused]] bool suppressVerbose) noexcept
{
    OS_Uint state = OS_THREAD_READY;

    // Get the current state of the thread
    if( OS_SUCCESS == os_thread_info_get(thread, &state) )
    {
		// Check if the thread is suspended
                if (os_thread_state_is_suspended(state))
		{
			// If the thread is suspended, attempt to resume it
                        OS_Uint err = os_thread_resume(thread);
			if (err == OS_SUCCESS)
			{
				os_delay_msec( 1 );   // Give chance for thread to execute
				return true;
			}
			else
			{
				// TBD: log appropriate error here
				return false;
			}
		}
		else
		{
			// If the thread is not suspended, there's nothing to do
			return true;
		}
    }
	else  // Unable to get state of thread
	{
		return false;
	}
}


bool os_thread_suspend_ex( OS_Thread* thread, [[maybe_unused]] bool suppressVerbose )
{
    if( thread != nullptr )
    {
        if( os_thread_is_live(thread) )
        {

            uint32_t result = os_thread_suspend(thread);

            os_delay_msec( 10 ); // Give the thread a chance to start

            if ( result == OS_SUCCESS)
            {
                return true;
            }
            else
            {
            }
        }
        else
        {
        }
    }
    else
    {
    }
    return false;
}

//============================================================================================//
// QUEUE
//============================================================================================//

/**
 * @brief Creates a queue with the specified parameters.
 *
 * @param queue - The OS queue to create
 * @param queueName - The name for the queue
 * @param messageSizeInWords - The size of the messages that will be stored in the queue
 */
bool os_queue_create_ex(OS_Queue& queue, const char* queueName, OS_Uint messageSizeInWords,
                   void* queueStorage, OS_Ulong queueSize, [[maybe_unused]] bool suppressVerbose) noexcept
{
    if(queueName != nullptr)
    {
        OS_Uint status = os_queue_create(&queue, const_cast<char*>(queueName), messageSizeInWords, queueStorage, queueSize);
        if (status == OS_SUCCESS)
        {
            return true;
        }
        else
        {
        }
    }
    else
    {
    }

    // TODO: Error Handling
    return false;
}

/**
 * @brief Deletes the specified queue and logs any error.
 *
 * @param queue - The OS queue to delete
 **/
bool os_queue_delete_ex(OS_Queue& queue, [[maybe_unused]] bool suppressVerbose) noexcept
{
    auto status = os_queue_delete(&queue);
    if(status != OS_SUCCESS)
    {
        if("queue")
        {
        }
        else
        {
        }
        return false;
    }

    return true;
}

/**
 * @brief Send a message to the queue.
 *
 * @param queue - The OS queue to send the message to
 * @param message - The message to send
 * @param wait_option - The option on how to wait if the queue is full
 */
bool os_queue_send_ex(OS_Queue& queue, void* message, OS_Ulong wait_option, [[maybe_unused]] bool suppressVerbose) noexcept {
    OS_Uint status = os_queue_send(&queue, message, wait_option);
    if (status == OS_SUCCESS) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Receive a message from the queue.
 *
 * @param queue - The OS queue to receive the message from
 * @param message - The received message
 * @param wait_option - The option on how to wait if the queue is empty
 */
bool os_queue_receive_ex(OS_Queue& queue, void* message, OS_Ulong wait_option, [[maybe_unused]] bool suppressVerbose) noexcept {
    OS_Uint status = os_queue_receive(&queue, message, wait_option);
    if (status == OS_SUCCESS) {
        return true;
    } else {
        return false;
    }
}

//============================================================================================//
// TIMER
//============================================================================================//

/**
  * @brief Function to create a OS timer and report errors associated with the creation
  *
  * @param timer Reference to timer control block.
  * @param name Pointer to timer name.
  * @param callback timer expiration callback function.
  * @param callbackExpirationInput A uint32_t data point that can be passed to expiration callback function.
  * @param initialTimeoutTicks Initial Expiration ticks.
  * @param rescheduleTimeoutTicks Reschedule ticks.
  * @param autoActivate The data value to set.
 */
bool os_timer_create_ex ( OS_Timer& timer, const char* name, void (*callback)(uint32_t),
                     uint32_t callbackExpirationInput, uint32_t initialTimeoutTicks,
                     uint32_t rescheduleTimeoutTicks, OS_Uint autoActivate,
                     [[maybe_unused]] bool suppressVerbose ) noexcept
{
	if( name != nullptr )
	{
		auto status = os_timer_create(&timer, const_cast<char*>(name), callback, callbackExpirationInput, initialTimeoutTicks,
				rescheduleTimeoutTicks, autoActivate);
		if (status == OS_SUCCESS)
		{
			return true;
		}
		else
		{
			os_delay_msec( 5 );
		}
	}
	else
	{
	}

	return false;
}



/**
  * @brief Function to delete a OS timer and report errors associated with the deletion
  *
  * @param timer The timer to delete.
  **/
bool os_timer_deactivate_and_delete_ex( OS_Timer& timer, [[maybe_unused]] bool suppressVerbose) noexcept
{
	// Stop the timer first
	auto status = os_timer_deactivate(&timer);
	if (status == OS_SUCCESS)
	{
		status = os_timer_delete(&timer);
		if (status != OS_SUCCESS)
		{
			if( "timer" )
			{
			}
			else
			{
			}
		}
	}
	else  // Failed to stop timer, usually due to back timer
	{
		// Time might have already been fired. No need to do any logging here
	}
	return status == OS_SUCCESS;
}

bool os_timer_activate_ex( OS_Timer& timer, [[maybe_unused]] bool suppressVerbose) noexcept
{
   auto status = os_timer_activate(&timer);
   if (status != OS_SUCCESS)
   {
		if( "timer" )
		{
		}
		else
		{
		}
   }
   else if( !suppressVerbose )
   {
	   if( "timer" )
	   {
	   }
	   else
	   {
	   }
   }
   return (status == OS_SUCCESS);
}

bool os_timer_deactivate_ex( OS_Timer& timer, [[maybe_unused]] bool suppressVerbose) noexcept
{
	auto status = os_timer_deactivate(&timer);
	if (status != OS_SUCCESS)
	{
		if( "timer" )
		{
		}
		else
		{
		}
	}
	else if( !suppressVerbose )
	{
		if( "timer" )
		{
		}
		else
		{
		}
	}

	return (status == OS_SUCCESS);
}

//============================================================================================//
// SEMAPHORES
//============================================================================================//

/**
 * @brief Function to Create TX semaphore.
 * @param txSemaphore Pointer to the OS semaphore control block.
 * @param name Name of the semaphore.
 * @param initial_count Initial count of the semaphore.
 * @return true if the semaphore was successfully created, false otherwise.
 */
bool os_semaphore_create_ex(OS_Semaphore* txSemaphore, const char* name, OS_Uint initial_count, [[maybe_unused]] bool suppressVerbose) noexcept
{
    OS_Uint err = os_semaphore_create(txSemaphore, const_cast<char*>(name), initial_count);

    if (err == OS_SUCCESS)
    {
        return true;
    }
    else
    {
        // TBD: log appropriate error here
        return false;
    }
}

/**
 * @brief Function to Delete TX semaphore.
 * @param txSemaphore Pointer to the OS semaphore control block to be deleted.
 * @return true if the semaphore was successfully deleted, false otherwise.
 */
bool os_semaphore_delete_ex(OS_Semaphore *txSemaphore, [[maybe_unused]] bool suppressVerbose) noexcept
{
    OS_Uint err = os_semaphore_delete(txSemaphore);

    if (err == OS_SUCCESS)
    {
        return true;
    }
    else
    {
        // TBD: log appropriate error here
        return false;
    }
}

/**
 * @brief Function to put (release) a TX semaphore.
 * @param txSemaphore Pointer to the OS semaphore control block.
 * @return true if the semaphore was successfully released, false otherwise.
 */
bool os_semaphore_put_ex(OS_Semaphore* txSemaphore, [[maybe_unused]] bool suppressVerbose) noexcept
{
    OS_Uint err = os_semaphore_put(txSemaphore);

    if (err == OS_SUCCESS)
    {
        return true;
    }
    else
    {
        // TBD: log appropriate error here
        return false;
    }
}

/**
 * @brief Function to get (wait for) a TX semaphore.
 * @param txSemaphore Pointer to the OS semaphore control block.
 * @param wait_option Specifies the maximum time to wait for the semaphore.
 * @return true if the semaphore was successfully acquired, false otherwise.
 */
bool os_semaphore_get_ex(OS_Semaphore* txSemaphore, OS_Ulong wait_option, [[maybe_unused]] bool suppressVerbose) noexcept
{
    OS_Uint err = os_semaphore_get(txSemaphore, wait_option);

    if (err == OS_SUCCESS)
    {
        return true;
    }
    else
    {
        // TBD: log appropriate error here
        return false;
    }
}

/**
 * @brief Function to get TX semaphore count.
 * @param txSemaphore Pointer to the OS semaphore control block.
 * @return the count of the semaphore.
 */
OS_Ulong os_semaphore_get_count_ex(OS_Semaphore *txSemaphore, [[maybe_unused]] bool suppressVerbose) noexcept
{
    OS_Ulong count;
    OS_Uint err = os_semaphore_info_get(txSemaphore, &count);

    if (err == OS_SUCCESS)
    {
        return count;
    }
    else
    {
        // TBD: log appropriate error here
        return 0;
    }
}


//============================================================================================//
// EVENTS
//============================================================================================//

/**
 * @brief Creates an event flag group with the specified parameters.
 *
 * @param eventFlags - The OS event flags to create
 * @param name - The name for the event flag group
 */
bool os_event_flags_create_ex(OS_EventGroup& eventFlags, const char* name, [[maybe_unused]] bool suppressVerbose) noexcept
{
    if(name != nullptr)
    {
        OS_Uint status = os_event_group_create(&eventFlags, const_cast<char*>(name));
        if (status == OS_SUCCESS)
        {
            return true;
        }
        else
        {
        }
    }
    else
    {
    }

    // TODO: Error Handling
    return false;
}

/**
 * @brief Deletes the specified event flag group and logs any error.
 *
 * @param eventFlags - The OS event flags to delete
 **/
bool os_event_flags_delete_ex(OS_EventGroup& eventFlags, [[maybe_unused]] bool suppressVerbose) noexcept
{
    auto status = os_event_group_delete(&eventFlags);
    if(status != OS_SUCCESS)
    {
        if("evt")
        {
        }
        else
        {
        }
        return false;
    }

    return true;
}

/**
 * @brief Set event flags in the event flag group.
 *
 * @param eventFlags - The OS event flags to set
 * @param flagsToSet - The flags to set
 */
bool os_event_flags_set_ex(OS_EventGroup& eventFlags, OS_Ulong flagsToSet, [[maybe_unused]] bool suppressVerbose) noexcept {
    OS_Uint status = os_event_group_set(&eventFlags, flagsToSet);
    if (status == OS_SUCCESS) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Clears event flags in the event flag group.
 *
 * @param eventFlags - The OS event flags to set
 * @param flagsToClear - The flags to clear
 */
bool os_event_flags_clear_ex(OS_EventGroup& eventFlags, OS_Ulong flagsToClear, [[maybe_unused]] bool suppressVerbose) noexcept {
    OS_Uint status = os_event_group_clear(&eventFlags, flagsToClear);
    if (status == OS_SUCCESS) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Get event flags from the event flag group.
 *
 * @param eventFlags - The OS event flags to get
 * @param flagsToGet - The flags to get
 * @param getOption - The option on how to get the flags
 * @param actualFlags - The actual flags gotten
 * @param wait_option - The option on how to wait if the flags are not available
 */
bool os_event_flags_get_ex(OS_EventGroup& eventFlags, OS_Ulong flagsToGet, OS_Uint getOption,
                     OS_Ulong& actualFlags, OS_Ulong wait_option, [[maybe_unused]] bool suppressVerbose) noexcept {
    OS_Uint status = os_event_group_get(&eventFlags, flagsToGet, getOption, &actualFlags, wait_option);
    if (status == OS_SUCCESS) {
        return true;
    } else {
        return false;
    }
}

//============================================================================================//
// STACK FAULT HANDLER
//============================================================================================//

void os_stack_fault_handler([[maybe_unused]] OS_Thread* thread)
{
}


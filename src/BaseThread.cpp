/**
 * @file BaseThread.cpp
 * @brief Implementation of the BaseThread class.
 *
 * This source file implements the BaseThread abstraction which provides the
 * common infrastructure for service and clinical threads. Refer to
 * BaseThread.h for a detailed description of the class design and usage.
 * @todo Add @copyright line once project copyright wording is finalised.
 */

#include "BaseThread.h"
#include "OsUtility.h"
#include "Utility.h"

//==============================================================//
/// BASE THREAD ITEMS BASE NAMES
//==============================================================//

/// Base thread start semaphore base osThreadName that will be pre-fixed on the thread Name

//==============================================================//
/// BASE THREAD
//==============================================================//

/**
  * @brief  The constructor initializes the thread and prepares it to start
  *  @return n/a
   */
BaseThread::BaseThread( const char* threadName) :
    initialized( false ),
	osThread{},
    osThreadName( threadName ),   // TODO:  This should be a strncpy into a buffer
	osThreadCreated( false),
	signalSemaphore( baseThreadStartSemaphoreBaseName, threadName),

	waitBeforeStep(0),
	minTimestampBeforeNextStep(0),
	threadRunning(false),
	threadStepInDelay(false),
	setupComplete(false),
	cleanupComplete(false),
	stopThreadRequested(false)
{
	/// No code at this moment.
}

/**
  * @brief  The destructor de-initializes the thread. It is not likely to be called in released products,
  * but will  support unit testing
  * @return n/a
  */
BaseThread::~BaseThread()
{
	if( osThreadCreated )
	{
		os_thread_delete_ex( &osThread);
	}
}

/**
 * @brief Returns the Base thread osThreadName.
 *
 * @return Returns the base thread osThreadName.
 */
const char* BaseThread::GetThreadName() noexcept
{

	return osThreadName;
}

/**
 * @brief Function to Create TX thread.
 * @param osThreadName Name of thread
 * @param stack - statically allocted stack
 * @param stackSizeBytes Numer of bytes allocated
 * @param priority - thread priority
 * @param preempt_threshold - thread preemption priority
 * @param timeSliceAllowed -
 *  @param auto_start -
 */
bool BaseThread::CreateBaseThread( uint8_t* stack, OS_Ulong stackSizeBytes,
                                   OS_Uint priority, OS_Uint preempt_threshold,
                                   OS_Ulong timeSliceAllowed, OS_Uint auto_start ) noexcept
{
    // Forward to the pinned variant with no affinity — preserves existing
    // ABI for every subclass that doesn't care about core affinity.
    return CreateBaseThread(stack, stackSizeBytes, priority, preempt_threshold,
                            timeSliceAllowed, auto_start, /*core_id*/ -1);
}

bool BaseThread::CreateBaseThread( uint8_t* stack, OS_Ulong stackSizeBytes,
                                   OS_Uint priority, OS_Uint preempt_threshold,
                                   OS_Ulong timeSliceAllowed, OS_Uint auto_start,
                                   int core_id ) noexcept
{
    /// Then, if the thread is not already created, create it
    if(signalSemaphore.EnsureInitialized())
    {
      /// Create the OS Thread (pinned to `core_id` if >= 0, no affinity otherwise).
           bool status = os_thread_create_ex_pinned(&osThread, const_cast<char*>(osThreadName), ThreadEntry,
                                        reinterpret_cast<OS_Ulong>(this), stack,
                                        stackSizeBytes, priority, preempt_threshold,
                                       timeSliceAllowed, auto_start, core_id );
	   /// If successfully created
	   if (status)
	   {
		   /// Mark the base thread as created.
		   osThreadCreated = true;

		return true;
	   }
	   else
	   {
		   os_delay_msec( 5 );
		   return false;
	   }
	   return osThreadCreated;
	}

	return false;
}

/**
  * @brief Suspend the underlying thread OS task.
  * @return True if action successful, false otherwise.
  */
bool BaseThread::Suspend() noexcept
{
	if( EnsureInitialized() )
	{
		return os_thread_suspend_ex( &osThread);
	}
	return false;
}

/**
  * @brief Resume the underlying thread OS task.
  * @return True if action successful, false otherwise.
  */
bool BaseThread::Resume() noexcept
{
	if( EnsureInitialized() )
	{
		return os_thread_resume_ex(&osThread);
	}
	return false;
}



/**
  * @brief Check the status of the underlying thread OS task.
  * @return True if action successful, false otherwise.
  */
bool BaseThread::IsSuspended() noexcept
{
        if( initialized )
        {
                OS_Uint state = 0;

	    // Get the current state of the thread
            if( OS_SUCCESS == os_thread_info_get(&osThread, &state) )
	    {
	    	// Check if the thread is suspended
                return os_thread_state_is_suspended(state);
	    }

	    return false;
	}
	else  // Not initialized?  consider it suspended
	{
		return true;
	}
}

bool BaseThread::Start() noexcept
{
	if(IsThreadRunning()) {
		return true;
	}

	/// Run start action
	if(StartAction()) {
	    /// If successful, signal the thread via the designated semaphore to start the process
	    return signalSemaphore.Signal();
	}

	/// Otherwise, indicate that we cannot start thread.
	return false;
}

bool BaseThread::StartAction() noexcept
{
	return true;
}


bool BaseThread::Stop() noexcept
{
    MarkThreadStopRequested();

    /// In case we are somehow suspended in our step, let's resume.
    os_thread_resume_if_suspended( &osThread );

    return true;
}

void BaseThread::ThreadEntry( OS_Ulong threadEntry)
{
    BaseThread* thread = reinterpret_cast<BaseThread*>(threadEntry);

    while ( true )
    {
        thread->ClearThreadRunning();   /// Clear the thread running flag so that we can Start() properly.

        thread->WaitForStart();         /// Wait for a signal to start ( it occurs in BaseThread Start() )

        thread->ResetVariables();		/// Reset all necessary variables

        thread->MarkThreadRunning();		/**< Thread should be marked as running immediately. */

        thread->ClearThreadStopRequested(); /**< Clear this flag in case CALLER didn't wait to send START command until the mode was completely stopped
                                                 and then sent another STOP flag because CALLER doesn't see the mode starting, this should not be the case
                                                 in the final product but necessary during development. */
        thread->ClearCleanupComplete();		/**< Clear the cleanup complete flag. */

        if( !thread->IsSetupComplete() )
        {
            thread->Setup();
            thread->MarkSetupComplete();
        }

        while ( !thread->IsThreadStopRequested() )
        {
        	thread->waitBeforeStep = thread->Step(); 											/// Main control loop sequence
        	thread->minTimestampBeforeNextStep = os_get_elapsed_time_msec() + thread->waitBeforeStep; /// Calculate next timestamp to Step() again

			thread->MarkThreadStepInDelay();							/// Mark that we are going to a delay
			os_delay_msec(static_cast<uint16_t>(thread->waitBeforeStep));	/// Delay requested time
			thread->ClearThreadStepInDelay();							/// Clear that we are in a delay
        }

        /// Make sure to mark
        if( !thread->IsCleanupComplete() )
        {
            thread->Cleanup();              /// Clean up before exiting the thread.
            thread->MarkCleanupComplete();	/// Let's set that the cleanup flag so that we can make that a cleanup is complete
        }

        thread->ClearSetupComplete();   /// Let's clear the setup complete flag so that a setup is ran before re-entering loop.

    }
}

/**
 * @brief Puts the caller thread in a waiting state until the service_start() is called.
 *
 */
void BaseThread::WaitForStart() noexcept
{
     signalSemaphore.WaitUntilSignalled();
}

bool BaseThread::StartThreadAndWaitToVerify(uint32_t startTimeoutMsec) {
    /// Start the thread
    if (Start()) {
        /// Function that will test if the thread has been started
        std::function<bool()> CheckIfThreadIsStarted = [this]() {
            return IsThreadRunning();
        };

        /// Expecting true within the timeout specified
        bool result = TestLogicWithTimeout(CheckIfThreadIsStarted, true, startTimeoutMsec, 10);

        return result; /// Return result
    }

    return false; /// Return false indicating Start() call failure
}

bool BaseThread::StopThreadAndWaitToVerify(uint32_t stopTimeoutMsec) {
    /// Stop the thread
    if (Stop())
    {
        /// Function that will test if the thread has been stopped
        std::function<bool()> CheckIfThreadIsStopped = [this]() {
            return IsThreadStopped();
        };

        /// Expecting true within the timeout specified
        bool result = TestLogicWithTimeout(CheckIfThreadIsStopped, true, stopTimeoutMsec, 10);

        return result; /// Return result
    }

    return false; /// Return false indicating Stop() call failure
}



uint32_t BaseThread::GetStackHighWaterMark() const noexcept
{
#if defined(HF_RTOS_FREERTOS)
    return uxTaskGetStackHighWaterMark(osThread);
#elif defined(HF_RTOS_THREADX)
    if (!os_thread_is_live(&osThread)) {
        return 0;
    }
    /* Unused bytes from the watermark pointer to stack end (when stack check is on). */
    if (osThread.tx_thread_stack_highest_ptr == TX_NULL ||
        osThread.tx_thread_stack_end == TX_NULL) {
        return 0;
    }
    const auto* highest = static_cast<const uint8_t*>(osThread.tx_thread_stack_highest_ptr);
    const auto* end = static_cast<const uint8_t*>(osThread.tx_thread_stack_end);
    if (end <= highest) {
        return 0;
    }
    return static_cast<uint32_t>(end - highest) / sizeof(uint32_t);
#else
    return 0;
#endif
}

bool BaseThread::ChangePriority(uint32_t newPriority) noexcept
{
#if defined(HF_RTOS_FREERTOS)
    if (osThread) {
        vTaskPrioritySet(osThread, newPriority);
        return true;
    }
    return false;
#elif defined(HF_RTOS_THREADX)
    if (!os_thread_is_live(&osThread)) {
        return false;
    }
    UINT old_priority = 0;
    const UINT err = tx_thread_priority_change(&osThread, static_cast<UINT>(newPriority),
                                               &old_priority);
    (void)old_priority;
    return err == TX_SUCCESS;
#else
    (void)newPriority;
    return false;
#endif
}

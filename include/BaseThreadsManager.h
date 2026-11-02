/**
 * @file BaseThreadsManager.h
 * @brief Optional registry that starts / stops a group of `BaseThread`
 *        instances together.
 *
 * Useful when several worker threads share a lifecycle (e.g. all
 * middleware sub-threads brought up at boot and torn down on shutdown).
 *
 * Thread-safety: the registry is guarded by an internal mutex; callers may
 * register / start / stop from any task context.
 *
 * Allocation: the registry uses `std::map`, so each registration allocates
 * once. The threads themselves never allocate stack — the caller supplies
 * each `BaseThread`'s stack buffer.
 *
 * @todo Add @copyright line once project copyright wording is finalised.
 */

#ifndef HF_UTILS_RTOS_WRAP_BASETHREADSMANAGER_H_
#define HF_UTILS_RTOS_WRAP_BASETHREADSMANAGER_H_

#include <algorithm>
#include <map>
#include <bitset>
#include <functional>

#include "OsAbstraction.h"

#include "OsUtility.h"
#include "Utility.h"
#include "BaseThread.h"
#include "Mutex.h"
#include "MutexGuard.h"

//==============================================================//
// CLASS
//==============================================================//
template <typename EnumType, EnumType MaxCount> class BaseThreadsManager
{
	public:
		BaseThreadsManager(const std::map<EnumType, BaseThread*>& threads, std::function<const char*(EnumType)> enumToStringFunc) ;

		/**
		  * @brief The Copy constructor is deleted to avoid copying instances.
	  	  * @return n/a
		  */

		BaseThreadsManager(const BaseThreadsManager&) = delete;


	    /**
	      * @brief  The assignment operator constructor is deleted to avoid copying instances.
	      * @return n/a
		  */

		BaseThreadsManager& operator = (const BaseThreadsManager&) = delete ;

		virtual ~BaseThreadsManager();

		bool EnsureInitialized() noexcept;

		bool ResumeAll() noexcept;

		bool ResumeSelected(const std::vector<EnumType>& selectedEnums) noexcept;

		bool StartAll() noexcept;

		bool StartSelected(const std::vector<EnumType>& selectedEnums) noexcept;

		bool StartAllExceptSelected(const std::vector<EnumType>& selectedEnums) noexcept;

		bool StartAllAndWaitToVerify(uint32_t waitToVerifyTimeoutMsec) noexcept;

		bool StartSelectedAndWaitToVerify(const std::vector<EnumType>& selectedEnums, uint32_t waitToVerifyTimeoutMsec) noexcept;

		bool StartAllExceptSelectedAndWaitToVerify(const std::vector<EnumType>& selectedEnums, uint32_t waitToVerifyTimeoutMsec) noexcept;

		bool StopAll() noexcept;

		bool StopSelected(const std::vector<EnumType>& selectedEnums) noexcept;

		bool StopAllExceptSelected(const std::vector<EnumType>& selectedEnums) noexcept;

		bool StopAllAndWaitToVerify(uint32_t waitToVerifyTimeoutMsec) noexcept;

		bool StopSelectedAndWaitToVerify(const std::vector<EnumType>& selectedEnums, uint32_t waitToVerifyTimeoutMsec) noexcept;

		bool StopAllExceptSelectedAndWaitToVerify(const std::vector<EnumType>& selectedEnums, uint32_t waitToVerifyTimeoutMsec) noexcept;

		virtual bool PreThreadInitializationActions() noexcept;

		virtual bool PostThreadInitializationActions() noexcept;

	private:

		/**
		 * @brief Initializes the BaseThreadsManager.
		 * @return True if the initialization is successful, false otherwise.
		 */
		bool Initialize() noexcept;

		//==============================================================//
		// VERBOSE??
		//==============================================================//
		static constexpr bool verbose = false;
		static constexpr bool timeBetweenChecksMsec = 10;

		bool initialized; /**< Flag indicating if the MwSubThreadsManager is initialized. */

		Mutex mutex;
		static const char mutexName[];

		std::map<EnumType, BaseThread*> threadsManaged;
		std::function<const char*(EnumType)> enumToString;

		std::bitset<MaxCount> threadsInitializedTracker;
		std::bitset<MaxCount> threadsStartedTracker;
		std::bitset<MaxCount> threadsStoppedTracker;
};


template <typename EnumType, EnumType MaxCount>
BaseThreadsManager<EnumType, MaxCount>::BaseThreadsManager(const std::map<EnumType, BaseThread*>& threads, std::function<const char*(EnumType)> enumToStringFunc) :
	initialized(false),
	mutex(mutexName),
	threadsManaged(threads),
	enumToString(enumToStringFunc),
	threadsInitializedTracker(),
	threadsStartedTracker(),
	threadsStoppedTracker()
{
	/// No code at this time.
}

template <typename EnumType, EnumType MaxCount>
BaseThreadsManager<EnumType, MaxCount>::~BaseThreadsManager() {
	}

/**
 * @brief Ensures that the BaseThreadsManager is initialized.
 *
 * This function checks if the BaseThreadsManager has been initialized and initializes it if not.
 *
 * @return true if the BaseThreadsManager is initialized, false otherwise.
 */
template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::EnsureInitialized() noexcept
{
	if (!initialized)
	{
		initialized = Initialize();
	}
	return initialized;
}

template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::ResumeAll() noexcept
{
	if (EnsureInitialized() )
	{
		MutexGuard guard(mutex);


		for(auto& threadMap : threadsManaged) {
			threadMap.second->Resume();
		}

		return true;
	}

	return false;
}
template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::ResumeSelected(const std::vector<EnumType>& selectedEnums) noexcept
{
	bool resumed = false;
	if (EnsureInitialized() )
	{
		MutexGuard guard(mutex);


		for(const auto& enumValue : selectedEnums)
		{
			auto threadMap = threadsManaged.find(enumValue);
			if(threadMap != threadsManaged.end())
			{
				resumed = threadMap->second->Resume();
			}
		}
	}

	return resumed;
}

template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::StartAll() noexcept
{
	 if (EnsureInitialized() )
	 {
		MutexGuard guard(mutex);


		for(auto& threadMap : threadsManaged) {
			threadsStartedTracker[threadMap.first] = threadMap.second->Start();
		}


		return threadsStartedTracker.all();
	}

	return false;
}


template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::StartSelected(const std::vector<EnumType>& selectedEnums) noexcept
{
	if (EnsureInitialized() )
	{
		MutexGuard guard(mutex);


		bool allSelectedStarted = true;
		for(const auto& enumValue : selectedEnums) {
			auto threadMap = threadsManaged.find(enumValue);
			if(threadMap != threadsManaged.end()) {
				bool started = threadMap->second->Start();
				allSelectedStarted = allSelectedStarted && started;
				threadsStartedTracker[threadMap->first] = started;
			}
		}


		return allSelectedStarted;
	}

	return false;
}

template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::StartAllExceptSelected(const std::vector<EnumType>& selectedEnums) noexcept
{
	if (EnsureInitialized() )
	{
		MutexGuard guard(mutex);


		bool allExceptSelectedStarted = true;
		/// Go through all threads managed
		for(const auto& threadMap : threadsManaged) {
			/// If it's not in the selected threads to not start
			if(std::find(selectedEnums.begin(), selectedEnums.end(), threadMap.first) == selectedEnums.end()) {
				/// Start Thread
				bool started = threadMap.second->Start();
				allExceptSelectedStarted = allExceptSelectedStarted && started;
				threadsStartedTracker[threadMap.first] = started;
			} else {
			}
		}


		return allExceptSelectedStarted;
	}

	return false;
}

template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::StartAllAndWaitToVerify(uint32_t waitToVerifyTimeoutMsec) noexcept
{
	if (EnsureInitialized() )
	{
		MutexGuard guard(mutex);


		for(auto& threadMap : threadsManaged) {
			threadMap.second->Start();
		}


		/// Function that will test if the thread has been started
		std::function<bool()> CheckIfAllHalSubThreadsAreStarted = [this]() {
			for(auto& threadMap : threadsManaged) {
				threadsStartedTracker[threadMap.first] = threadMap.second->IsThreadRunning();
			}
			return threadsStartedTracker.all();
		};

		const uint32_t waitStartTimeMsec = os_get_elapsed_time_msec();

		/// Expecting true within the timeout specified
		bool result = TestLogicWithTimeout(CheckIfAllHalSubThreadsAreStarted, true, waitToVerifyTimeoutMsec, timeBetweenChecksMsec);

		const uint32_t elapsedStartTimeMsec = os_get_elapsed_time_msec() - waitStartTimeMsec;

		/// Print info to user.

		if (result) {
		}
		else {
		}

		for(auto& threadMap : threadsManaged) {
		}

		return result; /// Return result
	}

	return false;
}

template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::StartSelectedAndWaitToVerify(const std::vector<EnumType>& selectedEnums, uint32_t waitToVerifyTimeoutMsec) noexcept
{
	if (EnsureInitialized() )
	{
		MutexGuard guard(mutex);



		for(const auto& enumValue : selectedEnums) {
			auto threadMap = threadsManaged.find(enumValue);
			if(threadMap != threadsManaged.end()) {
				threadMap->second->Start();
			}
		}



		/// Function that will test if the specified threads has been started
		std::function<bool()> CheckIfAllSelectedThreadsAreStarted = [this, selectedEnums] {
			/// Get if the selected threads are running
			for(const auto& enumValue : selectedEnums) {
				threadsStartedTracker[enumValue] = threadsManaged[enumValue]->IsThreadRunning();
			}
			/// Check if all specified subthreads have started
			for(const auto& enumValue : selectedEnums) {
				if(!threadsStartedTracker[enumValue]) {
					return false;
				}
			}
			return true;
		};

		const uint32_t waitStartTimeMsec = os_get_elapsed_time_msec();

		/// Expecting true within the timeout specified
		bool result = TestLogicWithTimeout(CheckIfAllSelectedThreadsAreStarted, true, waitToVerifyTimeoutMsec, timeBetweenChecksMsec);

		const uint32_t elapsedStartTimeMsec = os_get_elapsed_time_msec() - waitStartTimeMsec;

		/// Print info to user.

		if (result) {
		}
		else {
		}

		for(const auto& enumValue : selectedEnums) {
		}

		return result; /// Return result
	}

	return false;
}

template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::StartAllExceptSelectedAndWaitToVerify(const std::vector<EnumType>& selectedEnums, uint32_t waitToVerifyTimeoutMsec) noexcept
{
	if (EnsureInitialized() )
	{
		MutexGuard guard(mutex);



		/// For all threads managed
		for(const auto& threadMap : threadsManaged) {
			/// If they're not in the selected threads list indicated not to start
			if(std::find(selectedEnums.begin(), selectedEnums.end(), threadMap.first) == selectedEnums.end()) {
				/// Start threads
				threadMap.second->Start();
			}
		}


		/// Function that will test if the specified threads has been started
		std::function<bool()> CheckIfAllExceptSelectedThreadsAreStarted = [this, selectedEnums] {
			/// Get if the threads not selected are running
			for(const auto& threadMap : threadsManaged) {
				if(std::find(selectedEnums.begin(), selectedEnums.end(), threadMap.first) == selectedEnums.end()) {
					threadsStartedTracker[threadMap.first] = threadMap.second->IsThreadRunning();
				}
			}
			/// Check if all threads not selected have started
			for(const auto& threadMap : threadsManaged) {
				if(std::find(selectedEnums.begin(), selectedEnums.end(), threadMap.first) == selectedEnums.end()) {
					if(!threadsStartedTracker[threadMap.first]) {
						return false;
					}
				}
			}
			return true;
		};

		const uint32_t waitStartTimeMsec = os_get_elapsed_time_msec();

		/// Expecting true within the timeout specified
		bool result = TestLogicWithTimeout(CheckIfAllExceptSelectedThreadsAreStarted, true, waitToVerifyTimeoutMsec, timeBetweenChecksMsec);

		const uint32_t elapsedStartTimeMsec = os_get_elapsed_time_msec() - waitStartTimeMsec;

		/// Print info to user.

		if (result) {
		}
		else {
		}

		for(const auto& threadMap : threadsManaged) {
			if(std::find(selectedEnums.begin(), selectedEnums.end(), threadMap.first) == selectedEnums.end()) {
			}
		}

		return result; /// Return result
	}

	return false;
}

template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::StopAll() noexcept
{
	if (EnsureInitialized() )
	{
		MutexGuard guard(mutex);



		for(auto& threadMap : threadsManaged) {
			threadsStoppedTracker[threadMap.first] = threadMap.second->Stop();
		}


		return threadsStoppedTracker.all();
	}

	return false;
}
template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::StopSelected(const std::vector<EnumType>& selectedEnums) noexcept
{
	if (EnsureInitialized() )
	{
		MutexGuard guard(mutex);



		bool allSelectedStopped = true;
		for(const auto& enumValue : selectedEnums) {
			auto threadMap = threadsManaged.find(enumValue);
			if(threadMap != threadsManaged.end()) {
				bool stopped = threadMap->second->Stop();
				allSelectedStopped = allSelectedStopped && stopped;
				threadsStoppedTracker[threadMap->first] = stopped;
			}
		}


		return allSelectedStopped;
	}

	return false;
}

template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::StopAllExceptSelected(const std::vector<EnumType>& selectedEnums) noexcept
{
	if (EnsureInitialized() )
	{
		MutexGuard guard(mutex);



		bool allExceptSelectedStopped = true;
		for(const auto& threadMap : threadsManaged) {
			if(std::find(selectedEnums.begin(), selectedEnums.end(), threadMap.first) == selectedEnums.end()) {
				bool stopped = threadMap.second->Stop();
				allExceptSelectedStopped = allExceptSelectedStopped && stopped;
				threadsStoppedTracker[threadMap.first] = stopped;
			} else {
			}
		}


		return allExceptSelectedStopped;
	}

	return false;
}
template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::StopAllAndWaitToVerify(uint32_t waitToVerifyTimeoutMsec) noexcept
{
	if (EnsureInitialized() )
	{
		MutexGuard guard(mutex);


		for(auto& threadMap : threadsManaged) {
			threadsStoppedTracker[threadMap.first] = threadMap.second->Stop();
		}


		/// Function that will test if the thread has been started
		std::function<bool()> CheckIfAllHalSubThreadsAreStopped = [this]() {
			for(auto& threadMap : threadsManaged) {
				threadsStoppedTracker[threadMap.first] = threadMap.second->IsThreadStopped();
			}
			return threadsStoppedTracker.all();
		};

		/// Expecting true within the timeout specified
		const bool result = TestLogicWithTimeout(
		    CheckIfAllHalSubThreadsAreStopped, true, waitToVerifyTimeoutMsec, timeBetweenChecksMsec);
		return result;
	}

	return false;
}

template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::StopSelectedAndWaitToVerify(const std::vector<EnumType>& selectedEnums, uint32_t waitToVerifyTimeoutMsec) noexcept
{
	if (EnsureInitialized() )
	{
		MutexGuard guard(mutex);
;


		for(const auto& enumValue : selectedEnums) {
			auto threadMap = threadsManaged.find(enumValue);
			if(threadMap != threadsManaged.end()) {
				threadsStoppedTracker[threadMap->first] = threadMap->second->Stop();
			}
		}


		/// Function that will test if the specified threads has been stopped
		std::function<bool()> CheckIfAllSelectedThreadsAreStopped = [this, selectedEnums] {
			for(const auto& enumValue : selectedEnums) {
				threadsStoppedTracker[enumValue] = threadsManaged[enumValue]->IsThreadStopped();
			}
			for(const auto& enumValue : selectedEnums) {
				if(!threadsStoppedTracker[enumValue]) {
					return false;
				}
			}
			return true;
		};

		/// Expecting true within the timeout specified
		const bool result = TestLogicWithTimeout(
		    CheckIfAllSelectedThreadsAreStopped, true, waitToVerifyTimeoutMsec, timeBetweenChecksMsec);
		return result;
	}

	return false;
}


template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::StopAllExceptSelectedAndWaitToVerify(const std::vector<EnumType>& selectedEnums, uint32_t waitToVerifyTimeoutMsec) noexcept
{
	if (EnsureInitialized() )
	{
		MutexGuard guard(mutex);


		/// For all threads managed
		for(const auto& threadMap : threadsManaged) {
			/// If they're not in the selected threads list indicated not to start
			if(std::find(selectedEnums.begin(), selectedEnums.end(), threadMap.first) == selectedEnums.end()) {
				/// Start threads
				threadMap.second->Stop();
			} else {
			}
		}


		/// Function that will test if the specified threads has been stopped
		std::function<bool()> CheckIfAllExceptSelectedThreadsAreStopped = [this, selectedEnums] {
			/// Get if the threads not selected are stopped
			for(const auto& threadMap : threadsManaged) {
				if(std::find(selectedEnums.begin(), selectedEnums.end(), threadMap.first) == selectedEnums.end()) {
					threadsStoppedTracker[threadMap.first] = threadMap.second->IsThreadStopped();
				}
			}
			/// Check if all threads not selected have started
			for(const auto& threadMap : threadsManaged) {
				if(std::find(selectedEnums.begin(), selectedEnums.end(), threadMap.first) == selectedEnums.end()) {
					if(!threadsStoppedTracker[threadMap.first]) {
						return false;
					}
				}
			}
			return true;
		};

		const uint32_t waitStopTimeMsec = os_get_elapsed_time_msec();

		/// Expecting true within the timeout specified
		bool result = TestLogicWithTimeout(CheckIfAllExceptSelectedThreadsAreStopped, true, waitToVerifyTimeoutMsec, timeBetweenChecksMsec);

		const uint32_t elapsedStopTimeMsec = os_get_elapsed_time_msec() - waitStopTimeMsec;

		/// Print info to user.

		if (result) {
		}
		else {
		}

		for(const auto& threadMap : threadsManaged) {
			if(std::find(selectedEnums.begin(), selectedEnums.end(), threadMap.first) == selectedEnums.end()) {
			}
		}

		return result; /// Return result
	}

	return false;
}

template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::PreThreadInitializationActions() noexcept {
	return true;
}

template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::PostThreadInitializationActions() noexcept {
	return true;
}

/**
 * @brief Initializes the BaseThreadsManager.
 * @return True if the initialization is successful, false otherwise.
 */

template <typename EnumType, EnumType MaxCount>
bool BaseThreadsManager<EnumType, MaxCount>::Initialize() noexcept
{
	bool success = false;
	MutexGuard guard(mutex, &success);

	if( success )
	{
		/// If pre thread initialization actions fail, return false
		if( !PreThreadInitializationActions() ) { return false; }

		for(auto& threadMap : threadsManaged) {
			bool threadInitialized = threadMap.second->EnsureInitialized();
			threadsInitializedTracker[threadMap.first] = threadInitialized;

			if(threadInitialized) {
			}
			else {
				os_delay_msec( 5 );
			}
		}

		/// If post thread initialization actions fail, return false
		if( !PostThreadInitializationActions() ) { return false; }

		/// Otherwise, see if mutex and all threads managed are initialized.
	}
	return success && threadsInitializedTracker.all();
}



template <typename EnumType, EnumType MaxCount>
const char BaseThreadsManager<EnumType, MaxCount>::mutexName[] = "BaseThreadManager-Mutex";

#endif /* HF_UTILS_RTOS_WRAP_BASETHREADSMANAGER_H_ */

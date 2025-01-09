/*! \file

 *  Scheduling strategies used by the Interrupt Service RoutineA from Timer 2 (in scheduler.c)
 *  to determine which process may continue its execution next.

 *  The file contains two strategies:
 *  -round-robin
 *  -dynamic-priority-round-robin
*/

#include "os_scheduling_strategies.h"
#include "lib/defines.h"
#include "lib/ready_queue.h"
#include "lib/terminal.h"
#include <avr/pgmspace.h>



#include <stdbool.h>
#include <stdlib.h>

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

scheduling_information_t schedulingInfo; // initialization to 0 fits our needs

//----------------------------------------------------------------------------
// Given functions
//----------------------------------------------------------------------------

/*!
 *  Function used to determine whether there is any process ready (except the idle process)
 *
 *  \param processes[] The array of processes that it supposed to be looked through for processes that are ready
 *  \return True if there is a process ready which is not the idle proc
 */
bool isAnyProcReady(process_t const processes[])
{
	process_id_t i;
	for (i = 1; i < MAX_NUMBER_OF_PROCESSES; i++)
	{
		// The moment we find a single process that is ready/running, we can already return True
		if (processes[i].state == OS_PS_READY)
		{
			return true;
		}
	}

	// Not a single process that is ready/running has been found in the loop, so there is none
	return false;
}

//----------------------------------------------------------------------------
// Your Homework
//----------------------------------------------------------------------------

/*!
 *  This function implements the round-robin strategy. Every process gets the same
 *  amount of processing time and is rescheduled after each scheduler call
 *  if there are other processes running other than the idle process.
 *  The idle process is executed if no other process is ready for execution
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the even strategy.
 */
process_id_t os_scheduler_RoundRobin(process_t const processes[], process_id_t current)
{
	// If no process except idle process is ready, choose idle process
	if (!isAnyProcReady(processes))
	{
		return 0;
	}

	process_id_t nextProc = current;
	bool found = false;

	// Start searching for the next ready process, skip current process
	do
	{
		nextProc = (nextProc + 1) % MAX_NUMBER_OF_PROCESSES;
		if (processes[nextProc].state == OS_PS_READY && processes[nextProc].progID != 0)
		{
			found = true;
			break;
		}
	} while (nextProc != current);

	if (found)
	{
		return nextProc;
	}
	else if (processes[current].state == OS_PS_READY && processes[current].progID != 0)
	{
		// If no other process is ready, but current process is ready
		return current;
	}
	else
	{
		// If no process is ready, return idle process
		return 0;
	}
}



/*!
 * Reset the scheduling information for a specific process slot
 * This is necessary when a new process is started to clear out any
 * leftover data from a process that previously occupied that slot
 *
 * \param strategy The scheduling strategy currently in use
 * \param id  The process slot to erase state for
 */
void os_resetProcessSchedulingInformation(scheduling_strategy_t strategy, process_id_t id)
{
	if (strategy == OS_SS_DYNAMIC_PRIORITY_ROUND_ROBIN)
	{
		// Remove process from all ready queues
		for (uint8_t i = 0; i < PRIORITY_COUNT; i++)
		{
			rq_remove(&schedulingInfo.queues_ready[i], id);
		}
		process_t *process = os_getProcessSlot(id);
		// Enqueue process into its priority queue if it is in READY state
		priority_t priority = process->priority;
		if (process->state == OS_PS_READY)
		{
			rq_push(&schedulingInfo.queues_ready[priority], id);
				
		}
		
		
	}
	// For other strategies, do nothing
}

/*!
 *  Reset the scheduling information for a specific strategy
 *  This is only relevant for DynamicPriorityRoundRobin
 *  and is done when the strategy is changed through os_setSchedulingStrategy
 *
 * \param strategy  The strategy to reset information for
 */
void os_resetSchedulingInformation(scheduling_strategy_t strategy)
{
	if (strategy == OS_SS_DYNAMIC_PRIORITY_ROUND_ROBIN)
	{
		// Clear all ready queues
		for (uint8_t i = 0; i < PRIORITY_COUNT; i++)
		{
			rq_clear(&schedulingInfo.queues_ready[i]);
		}

		// Enqueue all READY processes into their respective priority queues
		for (process_id_t pid = 1; pid < MAX_NUMBER_OF_PROCESSES; pid++)
		{
			process_t *process = os_getProcessSlot(pid);
			if (process->state == OS_PS_READY)
			{
				priority_t priority = process->priority;
				rq_push(&schedulingInfo.queues_ready[priority], pid);
				
			}
		}
	}
	// For other strategies, do nothing
}




/*!
 *  This function implements the dynamic-priority-round-robin strategy.
 *  In this strategy, process priorities will matter that's achieved through multiple ready queues
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the dynamic priority round-robin strategy.
 */
process_id_t os_scheduler_DynamicPriorityRoundRobin(process_t const processes[], process_id_t current)
{
	
	
	// 1. Promote one process from each lower priority queue to the next higher priority queue
	
	if (!rq_isEmpty(&schedulingInfo.queues_ready[OS_PRIO_NORMAL]))
		{
			process_id_t pid = rq_pop(&schedulingInfo.queues_ready[OS_PRIO_NORMAL]); 
			rq_push(&schedulingInfo.queues_ready[OS_PRIO_HIGH],pid);
			
			
	}
	if (!rq_isEmpty(&schedulingInfo.queues_ready[OS_PRIO_LOW]))
	{
		process_id_t pid = rq_pop(&schedulingInfo.queues_ready[OS_PRIO_LOW]);
		rq_push(&schedulingInfo.queues_ready[OS_PRIO_NORMAL],pid);
		
		
	}
	

	// 2. Re-enqueue current process if READY
	if (current != 0 && processes[current].state == OS_PS_READY)
	{
		rq_push(&schedulingInfo.queues_ready[os_getProcessSlot(current)->priority], current);
		
	}
	
	if(!rq_isEmpty(&schedulingInfo.queues_ready[OS_PRIO_HIGH]))
	{
		return rq_pop(&schedulingInfo.queues_ready[OS_PRIO_HIGH]);
	}
	if(!rq_isEmpty(&schedulingInfo.queues_ready[OS_PRIO_NORMAL]))
	{
		return rq_pop(&schedulingInfo.queues_ready[OS_PRIO_NORMAL]);
	}
	if(!rq_isEmpty(&schedulingInfo.queues_ready[OS_PRIO_LOW]))
	{
		return rq_pop(&schedulingInfo.queues_ready[OS_PRIO_LOW]);
	}
	
	
	terminal_writeProgString(PSTR(" IDLE "));
	// 4. If no process is ready, return INVALID_PROCESS
	return 0;
}


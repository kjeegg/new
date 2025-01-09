/*! \file
 *
 *  Contains everything needed to realize the scheduling between multiple processes.
 *  Also contains functions to start the execution of programs or killing a process.
 *
 */

#include "os_scheduler.h"
#include "lib/lcd.h"
#include "lib/util.h"
#include "os_core.h"
#include "os_process.h"
#include "os_scheduling_strategies.h"

#include <avr/interrupt.h>
#include <stdbool.h>

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

//! Array of states for every possible process
process_t os_processes[MAX_NUMBER_OF_PROCESSES];

//! Array of function pointers for every registered program
program_t *os_programs[MAX_NUMBER_OF_PROGRAMS];

//! Index of process that is currently executed (default: idle)
process_id_t currentProc = 0;

//! currently active scheduling strategy
scheduling_strategy_t currSchedStrat = INITIAL_SCHEDULING_STRATEGY;

//! count of currently nested critical sections
uint8_t criticalSectionCount = 0;

//! Used to auto-execute programs.
uint16_t os_autostart;

//----------------------------------------------------------------------------
// Private function declarations
//----------------------------------------------------------------------------

//! ISR for timer compare match (scheduler)
ISR(TIMER2_COMPA_vect)
__attribute__((naked));

//! Wrapper to encapsulate processes
void os_dispatcher(void);

//! Casts a function pointer without throwing a warning
uint32_t addressOfProgram(program_t program);

//----------------------------------------------------------------------------
// Given functions
//----------------------------------------------------------------------------

/*!
 *  Casts a function pointer without throwing a warning.
 *
 *  \param program program pointer to convert
 *  \return converted uint32_t value
 */
uint32_t addressOfProgram(program_t program)
{
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
	return (uint32_t) program;
	#pragma GCC diagnostic pop
}

/*!
 *  Used to register a function as program. On success the program is written to
 *  the first free slot within the os_programs array (if the program is not yet
 *  registered) and the index is returned. On failure, INVALID_PROGRAM is returned.
 *  Note, that this function is not used to register the idle program.
 *
 *  \param program The function you want to register.
 *  \return The index of the newly registered program.
 */
program_id_t os_registerProgram(program_t *program)
{

	// Just check if idle-proc has been registered already
	assert(os_programs[0] != NULL, "Idle Proc not yet registered");

	uint8_t i = 0;

	// Check if the program is already registered and if yes, return the respective array slot
	for (; i < MAX_NUMBER_OF_PROGRAMS; i++)
	{
		if (program == os_programs[i])
		{
			return i;
		}
	}

	// If program is not yet registered...
	// Look for first free slot within os_programs array
	// Note that we start at 1, because this fct. is not used to register the idle proc
	for (i = 1; i < MAX_NUMBER_OF_PROGRAMS; i++)
	{
		if (os_programs[i] == NULL)
		{
			os_programs[i] = program;
			return i;
		}
	}

	// If the program is neither registered already nor is there a free slot, return INVALID_PROGRAM as an error indication
	return INVALID_PROGRAM;
}

/*!
 *  Used to check whether a certain program ID is to be automatically executed at
 *  system start.
 *
 *  \param programID The program to be checked.
 *  \return True if the program with the specified ID is to be started automatically.
 */
bool os_checkAutostartProgram(program_id_t programID)
{
	return (bool)(os_autostart & (1 << programID));
}

/*!
 * Lookup the main function of a program with id "programID".
 *
 * \param programID The id of the program to be looked up.
 * \return The pointer to the according function, or NULL if programID is invalid.
 */
program_t *os_lookupProgramFunction(program_id_t programID)
{
	// If the function pointer is null or the programID is invalid (i.e. too high)...
	if (programID >= MAX_NUMBER_OF_PROGRAMS)
	{
		
		return NULL;
	}
	
	// Else just return the respective function pointer
	return os_programs[programID];
}

/*!
 * Lookup the id of a program.
 *
 * \param program The function of the program you want to look up.
 * \return The id to the according slot, or INVALID_PROGRAM if program is invalid.
 */
program_id_t os_lookupProgramID(program_t *program)
{
	// Search program array for a match
	program_id_t i = 0;
	for (; i < MAX_NUMBER_OF_PROGRAMS; i++)
	{
		if (os_programs[i] == program)
		{
			return i;
		}
	}

	// If no match was found return INVALID_PROGRAM
	return INVALID_PROGRAM;
}

/*!
 *  A simple getter for the slot of a specific process.
 *
 *  \param pid The processID of the process to be handled
 *  \return A pointer to the memory of the process at position pid in the os_processes array.
 */
process_t *os_getProcessSlot(process_id_t pid)
{
	return os_processes + pid;
}

/*!
 *  A simple getter for the slot of a specific program.
 *
 *  \param programID The ProgramID of the process to be handled
 *  \return A pointer to the function pointer of the program at position programID in the os_programs array.
 */
program_t **os_getProgramSlot(program_id_t programID)
{
	return os_programs + programID;
}

/*!
 *  A simple getter to retrieve the currently active process.
 *
 *  \return The process id of the currently active process.
 */
process_id_t os_getCurrentProc(void)
{
	return currentProc;
}

/*!
 *  This function return the the number of currently active process-slots.
 *
 *  \returns The number currently active (not unused) process-slots.
 */
uint8_t os_getNumberOfActiveProcs(void)
{
	uint8_t num = 0;

	process_id_t i = 0;
	do
	{
		num += os_getProcessSlot(i)->state != OS_PS_UNUSED;
	} while (++i < MAX_NUMBER_OF_PROCESSES);

	return num;
}

/*!
 *  This function returns the number of currently registered programs.
 *
 *  \returns The amount of currently registered programs.
 */
uint8_t os_getNumberOfRegisteredPrograms(void)
{
	uint8_t i;
	for (i = 0; i < MAX_NUMBER_OF_PROGRAMS && *(os_getProgramSlot(i)); i++)
		;
	// Note that this only works because programs cannot be unregistered.
	return i;
}

/*!
 *  Sets the current scheduling strategy.
 *
 *  \param strategy The strategy that will be used after the function finishes.
 */
void os_setSchedulingStrategy(scheduling_strategy_t strategy)
{
	os_resetSchedulingInformation(strategy);
	currSchedStrat = strategy;
}

/*!
 *  This is a getter for retrieving the current scheduling strategy.
 *
 *  \return The current scheduling strategy.
 */
scheduling_strategy_t os_getSchedulingStrategy(void)
{
	return currSchedStrat;
}

/*!
 *  Enters a critical code section by disabling the scheduler if needed.
 *  This function stores the nesting depth of critical sections of the current
 *  process (e.g. if a function with a critical section is called from another
 *  critical section) to ensure correct behavior when leaving the section.
 *  This function supports up to 255 nested critical sections.
 */
void os_enterCriticalSection(void)
{
	// 1. Save global interrupt enable bit in local variable
	uint8_t ie = gbi(SREG, 7);

	// 2. Disable global interrupts (could also be done by sth. like SREG &=...)
	cli();

	// 3. Increment nesting depth of critical sections
	// Throw error if there are already too many critical sections entered
	// Note that it might be necessary to check for ==254 if os_error opens another critical section (not the case here...)
	if (criticalSectionCount == 255)
	{
		os_error("Crit. Section   overflow");
	}
	else
	{
		criticalSectionCount++;
	}

	// 4. Deactivate OCIE2A bit in TIMSK2 register to deactivate the TIMER2_COMPA_vect interrupt (i.e. our scheduler)
	cbi(TIMSK2, OCIE2A);
	

	// 5. Restore global interrupt enable bit: if it was disabled, nothing happens
	// If it was enabled, this operation does the same thing as "sei()"
	if (ie)
	{
		sei();
	}
}

/*!
 *  Leaves a critical code section by enabling the scheduler if needed.
 *  This function utilises the nesting depth of critical sections
 *  stored by os_enterCriticalSection to check if the scheduler
 *  has to be reactivated.
 */
void os_leaveCriticalSection(void)
{
	// 1. Save global interrupt enable bit in local variable
	uint8_t ie = gbi(SREG, 7);

	// 2. Disable global interrupts (could also be done by sth. like SREG &=...)
	cli();

	// 3. Decrement nesting depth of critical sections
	// Throw error if there is no critical Section that could be left
	if (criticalSectionCount == 0)
	{
		os_error("Crit. Section   underflow");
	}
	else
	{
		criticalSectionCount--;
	}

	// 4. Activate OCIE2A bit in TIMSK2 register if the last opened critical section is about to be closed
	if (criticalSectionCount == 0)
	{
		sbi(TIMSK2, OCIE2A);
		
	}

	// 5. Restore global interrupt enable bit: if it was disabled, nothing happens
	// If it was enabled, this operation does the same thing as "sei()"
	if (ie)
	{
		sei();
	}
}

//----------------------------------------------------------------------------
// Your Homework
//----------------------------------------------------------------------------

/*!
 *  Timer interrupt that implements our scheduler. Execution of the running
 *  process is suspended and the context saved to the stack.  If everything is
 *  in order, the next process for execution is derived with an exchangeable strategy.
 *  Finally the scheduler restores the next process for execution and releases control
 *  over the processor to that process.
 */
ISR(TIMER2_COMPA_vect)
{
	// 1. Save runtime context of current process
	saveContext();
	
	// 2. Save stack pointer of current process
	os_processes[currentProc].sp.as_int = SP;

	// 3. Set stack pointer to the scheduler stack (BOTTOM_OF_ISR_STACK)
	SP = BOTTOM_OF_ISR_STACK;

	// Now, check if the stack of currentProc is still in bounds
	if (!os_isStackInBounds(currentProc))
	{
		os_error("Stack overflow detected");
	}

	// Compute and store the checksum of the stack
	os_processes[currentProc].checksum = os_getStackChecksum(currentProc);

	// 4. Set process state to READY if it was previously RUNNING
	if (os_processes[currentProc].state == OS_PS_RUNNING) {
		os_processes[currentProc].state = OS_PS_READY;
	}

	// 5. Select the next process using the scheduling strategy
	if (os_getSchedulingStrategy() == OS_SS_DYNAMIC_PRIORITY_ROUND_ROBIN) {
		currentProc = os_scheduler_DynamicPriorityRoundRobin(os_processes, currentProc);
		} else {
		currentProc = os_scheduler_RoundRobin(os_processes, currentProc);
	}

	// If no ready processes are found, switch to idle process (PID 0)
	if (currentProc == INVALID_PROCESS) {
		currentProc = 0;  // Switch to idle process
	}

	// 6. Set the state of the next selected process to RUNNING
	os_processes[currentProc].state = OS_PS_RUNNING;

	// Now, before restoring the context, check the stack checksum
	// Recompute the checksum and compare with stored checksum
	uint8_t checksum = os_getStackChecksum(currentProc);
	if (checksum != os_processes[currentProc].checksum)
	{
		os_error("Stack corruption detected");
	}

	// Also, check if stack is in bounds
	if (!os_isStackInBounds(currentProc))
	{
		os_error("Stack overflow detected");
	}
	
	// 7. Set the stack pointer to the saved stack pointer of the new process
	SP = os_processes[currentProc].sp.as_int;
	
	// 8. Restore the runtime context of the new process
	restoreContext();
}


/*!
 *  This is the idle program. The idle process owns all the memory
 *  and processor time no other process wants to have.
 */
PROGRAM(0, AUTOSTART)
{
	
	while (true) {
		//lcd_clear();
		lcd_writeChar('.');
		delayMs(DEFAULT_OUTPUT_DELAY);
		
	}
}

/*!
 *  This function is used to execute a program that has been introduced with
 *  os_registerProgram.
 *  A stack will be provided if the process limit has not yet been reached.
 *
 *  \param programID The program id of the program to start (index of os_programs).
 *  \param priority Either one of OS_PRIO_LOW, OS_PRIO_NORMAL or OS_PRIO_HIGH
 *                  Note that the priority may be ignored by certain scheduling
 *                  strategies.
 *  \return The index of the new process (throws error on failure and returns
 *          INVALID_PROCESS as specified in defines.h).
 */
process_id_t os_exec(program_id_t programID, priority_t priority)
{
	// Enter critical section
	os_enterCriticalSection();

	// Find a free process slot
	process_id_t pid = 0;
	for (; pid < MAX_NUMBER_OF_PROCESSES; pid++)
	{
		if (os_processes[pid].state == OS_PS_UNUSED)
		{
			break;
		}
	}

	// No free slot found
	if (pid == MAX_NUMBER_OF_PROCESSES)
	{
		os_leaveCriticalSection();
		return INVALID_PROCESS;
	}

	// Validate program
	program_t *program = os_lookupProgramFunction(programID);
	if (program == NULL)
	{
		os_leaveCriticalSection();
		return INVALID_PROGRAM;
	}

	// Initialize process control block (PCB)
	os_processes[pid].progID = programID;
	os_processes[pid].state = OS_PS_READY;

	// Validate and set priority
	if (priority < OS_PRIO_HIGH || priority > OS_PRIO_LOW)
	{
		
		os_error("bruh");
	}
	os_processes[pid].priority = priority;

	// Initialize the stack pointer to the bottom of the process's stack
	os_processes[pid].sp.as_int = PROCESS_STACK_BOTTOM(pid);

	// Push the address of os_dispatcher onto the stack
	// Place the address of os_dispatcher on the stack
	uint32_t dispatcherAddress = addressOfProgram(os_dispatcher);
	*(os_processes[pid].sp.as_ptr--) = (uint8_t)(dispatcherAddress & 0xFF);          // Low byte
	*(os_processes[pid].sp.as_ptr--) = (uint8_t)((dispatcherAddress >> 8) & 0xFF);   // Middle byte
	*(os_processes[pid].sp.as_ptr--) = (uint8_t)((dispatcherAddress >> 16) & 0xFF);  // High byte
	
	

	// Initialize all 33 bytes in the context area to 0x00
	for (uint8_t i = 0; i < 33; i++)
	{
		*(os_processes[pid].sp.as_ptr--) = 0x00;
	}


	// Compute and store initial checksum
	os_processes[pid].checksum = os_getStackChecksum(pid);

	// Reset scheduling information for the new process
	os_resetProcessSchedulingInformation(os_getSchedulingStrategy(), pid);

	// Exit critical section
	os_leaveCriticalSection();
	
	

	return pid;
}


/*!
 *  In order for the scheduler to work properly, it must have the chance to
 *  initialize its internal data-structures and register and start the idle
 *  program.
 */
void os_initScheduler(void)
{
	

	// Initialize all processes as unused
	for (process_id_t pid = 0; pid < MAX_NUMBER_OF_PROCESSES; pid++) {
		os_processes[pid].state = OS_PS_UNUSED;  // Set process state to UNUSED
		
	}

	// Ensure idle process (PID 0) is registered and started first
	assert(os_programs[0] != NULL, "Idle process not registered");
	os_exec(0, OS_PRIO_LOW);

	

	// Auto-start all programs marked with AUTOSTART
	for (program_id_t progID = 1; progID < MAX_NUMBER_OF_PROGRAMS; progID++) {
		if (os_checkAutostartProgram(progID)) {
			
			os_exec(progID, DEFAULT_PRIORITY);
			
		}
	}
	os_resetSchedulingInformation(os_getSchedulingStrategy());
}

/*!
 *  If all processes have been registered for execution, the OS calls this
 *  function to start the concurrent execution of the applications.
 */
void os_startScheduler(void)
{

	// Set the current process to idle process (PID 0)
	currentProc = 0;

	// Set the state of the idle process to RUNNING
	os_processes[currentProc].state = OS_PS_RUNNING;

	
	
	// Set the stack pointer to the process stack of the idle process
	SP = os_processes[currentProc].sp.as_int;

	
	
	restoreContext();
	
	// If we return from restoreContext, it indicates an error
	os_error("Unexpected return from idle process");
}

/*!
 *  Calculates a spare checksum of the stack for a certain process, sampling only 16 equally distributed bytes of the whole process stack.
 *  First byte to sample is the stack's bottom.
 *
 *  \param pid The ID of the process for which the stack's checksum has to be calculated.
 *  \return The spare checksum of the pid'th stack.
 */
stack_checksum_t os_getStackChecksum(process_id_t pid)
{
	// Check if pid is valid
	if (pid >= MAX_NUMBER_OF_PROCESSES)
	return 0;

	uint8_t checksum = 0;

	// Get the stack pointer and the bottom of the process stack
	uint16_t stack_pointer = os_processes[pid].sp.as_int;
	uint16_t stack_bottom = PROCESS_STACK_BOTTOM(pid);

	// Ensure stack_pointer <= stack_bottom
	if (stack_pointer > stack_bottom)
	return 0;

	// Calculate the stack size
	uint16_t stack_size = stack_bottom - stack_pointer + 1; // +1 to include both ends

	// Number of samples is min(16, stack_size)
	uint8_t num_samples = 16;
	if (stack_size < 16)
	{
		num_samples = stack_size;
	}

	uint16_t step = 0;
	if (num_samples > 1)
	{
		step = (stack_size - 1) / (num_samples - 1);
	}
	else
	{
		step = 0;
	}

	uint16_t addr = stack_pointer;

	for (uint8_t i = 0; i < num_samples; i++)
	{
		uint8_t data = *((uint8_t *)addr);
		checksum ^= data;

		addr += step;

		if (addr > stack_bottom)
		addr = stack_bottom;
	}

	return checksum;
}


/*!
 *  Check if the stack pointer is still in its bounds
 *
 *  \param pid The ID of the process for which the stack pointer has to be checked.
 *  \return True if the stack pointer is still in its bounds.
 */
bool os_isStackInBounds(process_id_t pid)
{
    // Check if pid is valid
    if (pid >= MAX_NUMBER_OF_PROCESSES)
        return false;

    uint16_t stack_pointer = os_processes[pid].sp.as_int;
    uint16_t stack_bottom = PROCESS_STACK_BOTTOM(pid);
    uint16_t stack_limit = stack_bottom - STACK_SIZE_PROC + 1;

    // Check if stack_pointer is between stack_limit and stack_bottom
    if (stack_pointer >= stack_limit && stack_pointer <= stack_bottom)
        return true;
    else
        return false;
}




void os_yield()
{
	
	if(criticalSectionCount>0)
	{
		return;
	}
	
	// Disable global interrupts
	cli();

	// Reset timer counter TCNT2 to 0
	TCNT2 = 0;

	// Call the scheduler ISR directly
	TIMER2_COMPA_vect();

	// Global interrupts will be restored in ISR or after
	
}




/*!
 * Encapsulates any running process in order to make it possible for processes to terminate.
 *
 * This wrapper enables the possibility to perform a few necessary steps after the actual process
 * function has finished.
 */
void os_dispatcher() {
	process_id_t pid = os_getCurrentProc();
	

	program_t *program = os_lookupProgramFunction(os_processes[pid].progID);
	if (program == NULL) {
		
		os_kill(pid); 
	}
	
		
	program();
		
	
	os_kill(pid);
	

	while (1); //never
}




/*!
 *  Kills a process by cleaning up the corresponding slot in os_processes.
 *
 *  \param pid The process id of the process to be killed
 *  \return True, if the killing process was successful
 */
bool os_kill(process_id_t pid)
{
	// Check if pid is valid and not idle process
	if (pid >= MAX_NUMBER_OF_PROCESSES || pid == 0)
	{
		
		return false;
	}
	
	// Enter critical section
	os_enterCriticalSection();

	// Check if process is in use
	if (os_processes[pid].state == OS_PS_UNUSED)
	{
		
		os_leaveCriticalSection();
		return false;
	}

	if (os_processes[pid].state != OS_PS_UNUSED)
	{
		os_processes[pid].state = OS_PS_UNUSED;
		
	}

	// Reset scheduling information for this process
	os_resetProcessSchedulingInformation(os_getSchedulingStrategy(), pid);

	
	
	// If the current process is being killed
	if (pid == os_getCurrentProc())
	{
		// Close any remaining critical sections
		while (criticalSectionCount > 0)
		{
			os_leaveCriticalSection();
		}
		
		// Yield CPU, we must not return
		sei();
		os_yield();

		// Should not reach here
		while (1);
	}
	os_leaveCriticalSection();
	return true;
}
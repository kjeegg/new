#include "../progs.h"
#if defined(USER_PROGRAM_ENABLED) && USER_PROGRAM == 1

#include "../../lib/defines.h"
#include "../../lib/lcd.h"
#include "../../lib/util.h"
#include "../../os_core.h"
#include "../../os_process.h"

PROGRAM(4, AUTOSTART)
{
	os_enterCriticalSection();
	terminal_writeProgString(PSTR("test 4 autostart \n"));
	os_setSchedulingStrategy(OS_SS_DYNAMIC_PRIORITY_ROUND_ROBIN);
	os_exec(1, OS_PRIO_HIGH);
	os_exec(2, OS_PRIO_NORMAL);
	os_exec(3, OS_PRIO_LOW);
	os_leaveCriticalSection();
}

PROGRAM(1, DONTSTART)
{
	while (1)
	{
		lcd_writeChar('A');
		delayMs(DEFAULT_OUTPUT_DELAY);
	}
}

PROGRAM(2, DONTSTART)
{
	while (1)
	{
		lcd_writeChar('B');
		delayMs(DEFAULT_OUTPUT_DELAY);
	}
}

PROGRAM(3, DONTSTART)
{
	while (1)
	{
		lcd_writeChar('C');
		delayMs(DEFAULT_OUTPUT_DELAY);
	}
}

#endif
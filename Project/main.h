#ifndef _MAIN_H_
#define _MAIN_H_

void SetNextGameState(void (*init)(void), void (*update)(void), void (*exit)(void));

/// <summary>
/// Cleans up and closes the program
/// </summary>
void Terminate(void);
#endif // !_MAIN_H_


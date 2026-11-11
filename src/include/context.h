#pragma once

#include <task.h>

void context_switch(struct task *old, struct task *new);
/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Copyright (c) 2021 - PCB Arts GmbH
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

// C++ allocations (std::string, std::vector, new) use the FreeRTOS heap
// instead of the newlib heap. The newlib heap is only the few KB of RAM left
// after .bss, and its malloc has no lock while several tasks allocate (GUI
// task: solder profiles, SD task: profile list). heap_4 is thread-safe and
// also works before the scheduler starts (static constructors).

#include <cstdlib>
#include <new>

#include "FreeRTOS.h"

// new must return a unique pointer for size 0, heap_4 returns NULL for it
static void *allocate(std::size_t size)
{
  return pvPortMalloc(size != 0 ? size : 1);
}

void *operator new(std::size_t size)
{
  void *p = allocate(size);
  if (p == nullptr)
  {
    // no exceptions: same end as the library operator new
    abort();
  }
  return p;
}

void *operator new[](std::size_t size) { return operator new(size); }

void *operator new(std::size_t size, const std::nothrow_t &) noexcept
{
  return allocate(size);
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept
{
  return allocate(size);
}

void operator delete(void *p) noexcept { vPortFree(p); }

void operator delete[](void *p) noexcept { vPortFree(p); }

void operator delete(void *p, std::size_t) noexcept { vPortFree(p); }

void operator delete[](void *p, std::size_t) noexcept { vPortFree(p); }

void operator delete(void *p, const std::nothrow_t &) noexcept { vPortFree(p); }

void operator delete[](void *p, const std::nothrow_t &) noexcept
{
  vPortFree(p);
}

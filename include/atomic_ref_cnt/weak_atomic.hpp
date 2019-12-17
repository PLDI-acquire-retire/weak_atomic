
#ifndef WEAK_ATOMIC_H
#define WEAK_ATOMIC_H

#include <thread>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <experimental/optional>
#include <experimental/algorithm>
#include <atomic>
#include <stdio.h>
#include <string.h>

#include "weak_atomic_large.hpp"
#include "weak_atomic_small.hpp"
#include "acquire_retire_waitfree.hpp"

template <typename T, template<typename> typename AcquireRetire = AcquireRetireWaitfree>
using weak_atomic = typename std::conditional<(sizeof(T) <= 16), 
                                      weak_atomic_small<T, AcquireRetire>,
                                      weak_atomic_large<T, AcquireRetire> >::type;

#endif // WEAK_ATOMIC_H

/**
 * \file mutexunlocker.cpp
 * \brief Header file that defines the class MutexUnlocker
 * \author Alex Poirot <alexandre.poirot@legrand.fr>
**/

#include "mutexunlocker.hpp"

using namespace std;

MutexUnlocker::MutexUnlocker(mutex &mut) : mut(mut) {
	this->mut.lock();
}

MutexUnlocker::~MutexUnlocker() {
	this->mut.unlock();
}


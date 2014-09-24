/*
 * mutexunlocker.cpp
 *
 *  Created on: Sep 22, 2014
 *      Author: alex
 */

#include "mutexunlocker.hpp"

using namespace std;

MutexUnlocker::MutexUnlocker(mutex &mut) : mut(mut) {
	this->mut.lock();
}

MutexUnlocker::~MutexUnlocker() {
	this->mut.unlock();
}


/*
 * mutexunlocker.hpp
 *
 *  Created on: Sep 22, 2014
 *      Author: alex
 */

#ifndef MUTEXUNLOCKER_HPP_
#define MUTEXUNLOCKER_HPP_

#include <mutex>

class MutexUnlocker {
public:
	MutexUnlocker(std::mutex &mut);
	virtual ~MutexUnlocker();

private :
	std::mutex &mut;
};

#endif /* MUTEXUNLOCKER_HPP_ */

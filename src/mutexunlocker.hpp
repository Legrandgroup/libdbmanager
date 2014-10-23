/**
 * \file mutexunlocker.hpp
 * \brief Header file that defines the class MutexUnlocker
 * \author Alex Poirot <alexandre.poirot@legrand.fr>
**/

#ifndef MUTEXUNLOCKER_HPP_
#define MUTEXUNLOCKER_HPP_

#include <mutex>

/**
 * \class MutexUnlocker
 * \brief This class is used to automatically release a lock when the object goes out of scope
**/
class MutexUnlocker {
public:
	/**
	 * \brief Constructor
	 * The only constructor of the class... will grab the lock \p mut when invoked
	 *
	 * \param mut The mutex to hold
	**/
	MutexUnlocker(std::mutex &mut);
	/**
	 * \brief Destructor
	 * Destructor of the class... will release the lock when invoked
	**/
	virtual ~MutexUnlocker();

private:
	std::mutex &mut;	/*!< The mutex encapsulated in this object */
};

#endif /* MUTEXUNLOCKER_HPP_ */

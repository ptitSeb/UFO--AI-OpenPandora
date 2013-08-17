#ifndef COUNTERMANAGER_H_
#define COUNTERMANAGER_H_

#include "iradiant.h"
#include "scenelib.h"
#include "generic/callback.h"
#include "string/string.h"

#include <stdexcept>

namespace map {

class SimpleCounter: public Counter
{
		Callback m_countChanged;
		std::size_t m_count;
	public:
		SimpleCounter () :
			m_count(0)
		{
		}

		void setCountChangedCallback (const Callback& countChanged)
		{
			m_countChanged = countChanged;
		}

		void increment ()
		{
			++m_count;
			m_countChanged();
		}

		void decrement ()
		{
			--m_count;
			m_countChanged();
		}

		std::size_t get () const
		{
			return m_count;
		}
};

class CounterManager
{
		typedef SimpleCounter* CounterPtr;
		typedef std::map<CounterType, CounterPtr> CounterMap;
		CounterMap _counters;
	public:
		CounterManager ()
		{
			createCounter(counterBrushes);
			createCounter(counterEntities);
		}

		void createCounter (CounterType counter)
		{
			// Create the counter object
			_counters[counter] = CounterPtr(new SimpleCounter);
			// And connect the changed signal to this class
			_counters[counter]->setCountChangedCallback(
					MemberCaller<CounterManager, &CounterManager::updateStatusBar> (*this));
		}

		~CounterManager ()
		{
			for (CounterMap::iterator i = _counters.begin(); i != _counters.end(); ++i) {
				delete i->second;
			}
		}

		Counter& get (CounterType counter)
		{
			if (_counters.find(counter) == _counters.end()) {
				throw std::runtime_error("Counter ID not found.");
			}
			return *_counters[counter];
		}

		void updateStatusBar ()
		{
			int brushCount(_counters[counterBrushes]->get());
			int entityCount(_counters[counterEntities]->get());

			std::string text = "Brushes: " + string::toString(brushCount);
			text += " Entities: " + string::toString(entityCount);

			GlobalRadiant().setStatusText(text);
		}
};

} // namespace map

#endif /*COUNTERMANAGER_H_*/

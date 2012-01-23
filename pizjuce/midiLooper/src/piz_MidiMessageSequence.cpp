#include "piz_MidiMessageSequence.h"


//==============================================================================
PizMidiMessageSequence::PizMidiMessageSequence()
{
}

PizMidiMessageSequence::PizMidiMessageSequence (const PizMidiMessageSequence& other)
{
    //list.ensureStorageAllocated (other.list.size());

    for (int i = 0; i < other.list.size(); ++i)
        list.add (new MidiEventHolder (other.list.getUnchecked(i)->message));
}

PizMidiMessageSequence::PizMidiMessageSequence (const MidiMessageSequence& other)
{
    //list.ensureStorageAllocated (other.getNumEvents());

    for (int i = 0; i < other.getNumEvents(); ++i)
        list.add (new MidiEventHolder (other.getEventPointer(i)->message));
}

PizMidiMessageSequence& PizMidiMessageSequence::operator= (const PizMidiMessageSequence& other)
{
    PizMidiMessageSequence otherCopy (other);
    swapWith (otherCopy);
    return *this;
}

PizMidiMessageSequence& PizMidiMessageSequence::operator= (const MidiMessageSequence& other)
{
    PizMidiMessageSequence otherCopy (other);
    swapWith (otherCopy);
    return *this;
}

MidiMessageSequence PizMidiMessageSequence::getAsJuceSequence ()
{
    MidiMessageSequence copy;
    for (int i = 0; i < list.size(); ++i)
        copy.addEvent (list.getUnchecked(i)->message);

    return copy;
}

void PizMidiMessageSequence::swapWith (PizMidiMessageSequence& other) throw()
{
    list.swapWithArray (other.list);
}

PizMidiMessageSequence::~PizMidiMessageSequence()
{
}

void PizMidiMessageSequence::clear()
{
    list.clear();
}

int PizMidiMessageSequence::getNumEvents() const
{
    return list.size();
}

PizMidiMessageSequence::mehPtr PizMidiMessageSequence::getEventPointer (const int index) const
{
    return list [index];
}

double PizMidiMessageSequence::getTimeOfMatchingKeyUp (const int index) const
{
    const mehPtr meh = list [index];

	if (meh != 0 && meh->noteOffObject != 0) {
		jassert(meh->noteOffObject->message.getTimeStamp() >= meh->message.getTimeStamp());
        return meh->noteOffObject->message.getTimeStamp();
	}
    else if (meh!=0 && meh->message.isNoteOn()) 
		return meh->message.getTimeStamp()+1;
	return 0.0;
}

int PizMidiMessageSequence::getIndexOfMatchingKeyUp (const int index) const
{
    const mehPtr meh = list [index];

    return (meh != 0) ? list.indexOf (meh->noteOffObject) : -1;
}

int PizMidiMessageSequence::getIndexOf (mehPtr const event) const
{
    return list.indexOf (event);
}

int PizMidiMessageSequence::getNextIndexAtTime (const double timeStamp) const
{
    const int numEvents = list.size();

    int i;
    for (i = 0; i < numEvents; ++i)
        if (list.getUnchecked(i)->message.getTimeStamp() >= timeStamp)
            break;

    return i;
}

//==============================================================================
double PizMidiMessageSequence::getStartTime() const
{
    if (list.size() > 0)
        return list.getUnchecked(0)->message.getTimeStamp();
    else
        return 0;
}

double PizMidiMessageSequence::getEndTime() const
{
    if (list.size() > 0)
        return list.getLast()->message.getTimeStamp();
    else
        return 0;
}

double PizMidiMessageSequence::getEventTime (const int index) const
{
    if (((unsigned int) index) < (unsigned int) list.size())
        return list.getUnchecked (index)->message.getTimeStamp();

    return 0.0;
}

//==============================================================================
void PizMidiMessageSequence::addEvent (const MidiMessage& newMessage,
                                    double timeAdjustment)
{
    mehPtr const newOne = new MidiEventHolder (newMessage);

    timeAdjustment += newMessage.getTimeStamp();
    newOne->message.setTimeStamp (timeAdjustment);

    int i;
    for (i = list.size(); --i >= 0;)
        if (list.getUnchecked(i)->message.getTimeStamp() <= timeAdjustment)
            break;

    list.insert (i + 1, newOne);
}

//==============================================================================
void PizMidiMessageSequence::moveEvent (int index, double timeAdjustment, const bool moveMatchingNoteUp)
{
    if (((unsigned int) index) < (unsigned int) list.size())
	{
		MidiMessage &message1 = list.getUnchecked(index)->message;
		message1.addToTimeStamp (timeAdjustment);
		if (moveMatchingNoteUp)
		{
			MidiMessage &message2 = list.getUnchecked(index)->noteOffObject->message;
			timeAdjustment += message2.getTimeStamp();
			message2.addToTimeStamp (timeAdjustment);
		}
		sort();
	}
}

void PizMidiMessageSequence::deleteEvent (const int index,
                                       const bool deleteMatchingNoteUp)
{
    if (((unsigned int) index) < (unsigned int) list.size())
    {
        if (deleteMatchingNoteUp)
            deleteEvent (getIndexOfMatchingKeyUp (index), false);

        list.remove (index);
    }
}

void PizMidiMessageSequence::addSequence (const PizMidiMessageSequence& other,
                                       double timeAdjustment,
                                       double firstAllowableTime,
                                       double endOfAllowableDestTimes)
{
    firstAllowableTime -= timeAdjustment;
    endOfAllowableDestTimes -= timeAdjustment;

    for (int i = 0; i < other.list.size(); ++i)
    {
        const MidiMessage& m = other.list.getUnchecked(i)->message;
        const double t = m.getTimeStamp();

        if (t >= firstAllowableTime && t < endOfAllowableDestTimes)
        {
            mehPtr const newOne = new MidiEventHolder (m);
            newOne->message.setTimeStamp (timeAdjustment + t);

            list.add (newOne);
        }
    }

    sort();
}

void PizMidiMessageSequence::addSequence (const PizMidiMessageSequence& other)
{
    for (int i = 0; i < other.list.size(); ++i)
    {
        const MidiMessage& m = other.list.getUnchecked(i)->message;
        const double t = m.getTimeStamp();

        mehPtr const newOne = new MidiEventHolder (m);
        newOne->message.setTimeStamp (t);

        list.add (newOne);
    }

    sort();
}

//==============================================================================
int PizMidiMessageSequence::compareElements (const mehPtr first,
                                          const mehPtr second) throw()
{
    const double diff = first->message.getTimeStamp()
                         - second->message.getTimeStamp();

    return (diff > 0) - (diff < 0);
}

void PizMidiMessageSequence::sort()
{
    list.sort (*this, true);
}

//==============================================================================
void PizMidiMessageSequence::updateMatchedPairs(bool sortEvents)
{
    //DBG("start updateMatchedPairs()");
	if (sortEvents) sort();
	for (int i = 0; i < list.size(); ++i)
    {
        const MidiMessage& m1 = list.getUnchecked(i)->message;

        if (m1.isNoteOn())
        {
            list.getUnchecked(i)->noteOffObject = 0;
            const int note = m1.getNoteNumber();
            const int chan = m1.getChannel();
            const int len = list.size();

            for (int j = i + 1; j < len; ++j)
            {
                const MidiMessage& m = list.getUnchecked(j)->message;

                if (m.getNoteNumber() == note && m.getChannel() == chan)
                {
                    if (m.isNoteOff())
                    {
                        list.getUnchecked(i)->noteOffObject = list[j];
                        break;
                    }
                    else if (m.isNoteOn())
                    {
                        //list.insert (j, new MidiEventHolder (MidiMessage::noteOff (chan, note)));
                        //list.getUnchecked(j)->message.setTimeStamp (m.getTimeStamp());
                        //list.getUnchecked(i)->noteOffObject = list[j];
                        //break;
                    }
                }
            }
			//jassert(list.getUnchecked(i)->noteOffObject!=0);
        }
    }
    //DBG("end updateMatchedPairs()");
}

void PizMidiMessageSequence::addTimeToMessages (const double delta)
{
    for (int i = list.size(); --i >= 0;)
        list.getUnchecked (i)->message.setTimeStamp (list.getUnchecked (i)->message.getTimeStamp()
                                                      + delta);
}

//==============================================================================
void PizMidiMessageSequence::extractMidiChannelMessages (const int channelNumberToExtract,
                                                      PizMidiMessageSequence& destSequence,
                                                      const bool alsoIncludeMetaEvents) const
{
    for (int i = 0; i < list.size(); ++i)
    {
        const MidiMessage& mm = list.getUnchecked(i)->message;

        if (mm.isForChannel (channelNumberToExtract)
             || (alsoIncludeMetaEvents && mm.isMetaEvent()))
        {
            destSequence.addEvent (mm);
        }
    }
}

void PizMidiMessageSequence::extractSysExMessages (PizMidiMessageSequence& destSequence) const
{
    for (int i = 0; i < list.size(); ++i)
    {
        const MidiMessage& mm = list.getUnchecked(i)->message;

        if (mm.isSysEx())
            destSequence.addEvent (mm);
    }
}

void PizMidiMessageSequence::deleteMidiChannelMessages (const int channelNumberToRemove)
{
    for (int i = list.size(); --i >= 0;)
        if (list.getUnchecked(i)->message.isForChannel (channelNumberToRemove))
            list.remove(i);
}

void PizMidiMessageSequence::deleteSysExMessages()
{
    for (int i = list.size(); --i >= 0;)
        if (list.getUnchecked(i)->message.isSysEx())
            list.remove(i);
}

//==============================================================================
void PizMidiMessageSequence::createControllerUpdatesForTime (const int channelNumber,
                                                          const double time,
                                                          OwnedArray<MidiMessage>& dest)
{
    bool doneProg = false;
    bool donePitchWheel = false;
    Array <int> doneControllers;
    doneControllers.ensureStorageAllocated (32);

    for (int i = list.size(); --i >= 0;)
    {
        const MidiMessage& mm = list.getUnchecked(i)->message;

        if (mm.isForChannel (channelNumber)
             && mm.getTimeStamp() <= time)
        {
            if (mm.isProgramChange())
            {
                if (! doneProg)
                {
                    dest.add (new MidiMessage (mm, 0.0));
                    doneProg = true;
                }
            }
            else if (mm.isController())
            {
                if (! doneControllers.contains (mm.getControllerNumber()))
                {
                    dest.add (new MidiMessage (mm, 0.0));
                    doneControllers.add (mm.getControllerNumber());
                }
            }
            else if (mm.isPitchWheel())
            {
                if (! donePitchWheel)
                {
                    dest.add (new MidiMessage (mm, 0.0));
                    donePitchWheel = true;
                }
            }
        }
    }
}


//==============================================================================
PizMidiMessageSequence::MidiEventHolder::MidiEventHolder (const MidiMessage& message_)
   : message (message_),
     noteOffObject (0)
{
}

PizMidiMessageSequence::MidiEventHolder::~MidiEventHolder()
{
}


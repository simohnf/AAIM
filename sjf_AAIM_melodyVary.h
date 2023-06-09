//
//  sjf_AAIM_melodyVary.h
//  
//
//  Created by Simon Fay on 21/05/2023.
//

#ifndef sjf_AAIM_melodyVary_h
#define sjf_AAIM_melodyVary_h
#include "sjf_audio/sjf_audioUtilitiesC++.h"

/*
 Class to map AAIM_rhythmGen output onto a melodic pattern
              CAVEATS
    - There is an assumption that the melody contains 12 or fewer pitch classes...
    - This only works for monophonic lines...
 */
template< class T >
class AAIM_melodyVary
{
public:
    AAIM_melodyVary()
    {
        setNumBeats( m_nBeats );
        m_availablePitchClasses.reserve( 12 );
        m_availablePitchClasses.clear();
    }
    ~AAIM_melodyVary(){}
    
    /*
     input melody as vector of std::array< size_t, 2 >
            melody[ i ][ 0 ] = pitch
            melody[ i ][ 1 ] = triggerType
     */
    void setMelody( std::vector< std::array< size_t, 2 > > melody )
    {
        auto nNotes = melody.size();
        if ( nNotes < 1 )
            return;
        setNumBeats( nNotes );
        m_melody.clear();
//        m_melody.reserve( nNotes );
        // just a safety run through first to make sure everything is within range ( I might remove this... )
        for ( size_t i = 0; i < m_nBeats; i++ )
        {
            melody[ i ][ 0 ] = melody[ i ][ 0 ] < 0 ? 0 : melody[ i ][ 0 ];
            melody[ i ][ 1 ] = melody[ i ][ 1 ] < rest || melody[ i ][ 1 ] > newNote ? rest : melody[ i ][ 1 ];
        }
        m_melodyRaw = melody; // store for future use?
        convertRawMelodyToAAIM_Notes( m_melodyRaw );
    }
    
    std::vector< std::array< size_t, 3 > > getMelody()
    {
        auto size = m_melody.size();
        auto melody = std::vector< std::array< size_t, 3 > >( size );
        melody.clear();
        for ( size_t i = 0; i < size; i ++ )
        {
            auto note = m_melody[ i ];
            melody.emplace_back( std::array< size_t, 3 >{ note.getPosition(), note.getPitch(), note.getLength() } );
        }
        return melody;
    }
    
    void setNumBeats( size_t nBeats )
    {
        m_nBeats = nBeats < 1 ? 1 : nBeats;
        m_melody.reserve( m_nBeats );
        m_melodyRaw.reserve( m_nBeats );
    }
    
    size_t getNumBeats( )
    {
        return m_nBeats;
    }
    
    void setAvailablePitchClasses( std::vector< int > pitchClasses)
    {
        auto pitchClassFlags  = std::array< bool, 12 >{};
        m_availablePitchClasses.clear();
        for ( size_t i = 0; i < pitchClasses.size(); i++ )
        {
            auto pitchClass = fastMod4<int>( pitchClasses[ i ], 12 );
            pitchClassFlags[ pitchClass ] = true;
        }
        for ( size_t i = 0; i < pitchClassFlags.size(); i++ )
        {
            if ( pitchClassFlags[ i ] )
                m_availablePitchClasses.emplace_back( i );
        }
    }
    
    std::vector< int > getAvailablePitchClasses()
    {
        return m_availablePitchClasses;
    }
    // receive a trigger with current position and beats to sync and map this onto the original melody
    std::array< T, 2 > triggerPitch( T currentBeat, T beatsToSync = 1 )
    {
        auto outNote = std::array< T, 2 >{ -1, 0 };
        for ( size_t i = 0; i < m_melody.size(); i++ )
        {
            if ( m_melody[ i ].getPosition() == static_cast<int>(currentBeat) )
            {
                outNote = { static_cast<T>( m_melody[ i ].getPitch() ), static_cast<T>( m_melody[ i ].getLength() ) };
                break;
            }
        }
        return outNote;
//        // check distance from current position to nearest note in pattern
//        auto distance = (T)m_nBeats;
//        for ( size_t i = 0; i < m_nBeats + 1; i++ ) // add one to total nBeats to circle to beginning of pattern at end
//        {
//            auto distFromPos = abs(currentBeat - i);
//            distance = ( m_melody[ i % m_nBeats ]. && distFromPos < distance ) ? distFromPos : distance;
//        }
//        // if distance <= baseIOI && < random01
//        // output trigger
//        if ( distance < beatsToSync && (distance / beatsToSync) < rand01() )
//            return true;
//        // count the IOIs in pattern and divide by total number of beats
//        auto count = 0;
//        for ( size_t i = 0; i < m_nBeats; i++ )
//            count = m_pattern[ i ] ? count + 1 : count;
//        auto fract = (T)count / (T)m_nBeats; // calculate fraction of pattern that has beats
//        return ( fract*m_fills > rand01() ) ? true : false; // if ( fraction * fills probability ) is greater than a random number output a beat
    }
    
    enum triggerTypes
    {
        rest = 0, tie, newNote
    };
    
private:
    class AAIM_Note
    {
    public:
        AAIM_Note( size_t position, size_t pitch, size_t length )
        :   m_position{ position},
            m_pitch { pitch },
            m_length { length }
        { }
        ~AAIM_Note(){}

        void setPosition( size_t position ){ m_position = position; }
        size_t getPosition(){ return m_position; }

        void setPitch( size_t pitch ){ m_pitch = pitch; }
        size_t getPitch(){ return m_pitch; }
        
        void setLength( size_t length ){ m_length = length; }
        size_t getLength(){ return m_length; }
        
    private:
        size_t m_position{}, m_pitch{}, m_length{};
    };
    
    void convertRawMelodyToAAIM_Notes( std::vector< std::array< size_t, 2 > > melody )
    {
        if ( m_nBeats == 1 )
        {
            m_melody.emplace_back( AAIM_Note{ 0, melody[ 0 ][ 0 ], 1 } );
            calculateAvailblePitches( m_melody );
            return;
        }
        auto start = 0ul;
        // find the first new note in the pattern
        // This just checks to see if there was a tie over the bar line
        // first check beat one
        if ( melody[ 0 ][ 1 ] == tie && melody[ (m_nBeats - 1) ][ 0 ] == melody[ 0 ][ 0 ] )
        {
            // then check last beat etc. etc.
            for ( size_t i = 1; i < m_nBeats; i++ )
            {
                auto previousIndex = i - 1;
                previousIndex  = fastMod4< size_t >( previousIndex, m_nBeats );
                
                if (
                    melody[ i ][ 0 ] != melody[ previousIndex ][ 0 ] // if the pitch changes
                    || melody[ i ][ 1 ] == newNote // or the trigger type is a new note
                    || melody[ previousIndex ][ 1 ] == rest // or the previous trigger type is a rest
                    )
                {
                    start = i; // the first "new note in the pattern
                    break;
                }
            }
        }

        // convert the pitch, trigger type pairings to [ position, pitch, length ]
        // starting at start posize_t
        //      position is start posize_t
        //      pitch is melody[ start ][ 0 ]
        //      calculate length
        
        auto count = 0ul;
        for ( size_t i = 0; i < m_nBeats; i++ )
        {
            auto position = fastMod4< size_t >( i+start, m_nBeats );
            auto pitch = melody[ position ][ 0 ];
            auto trig = melody[ position ][ 1 ];
            auto length = (trig == rest) ? 0 : 1ul;
            if( i == 0 )
            {
                m_melody.emplace_back( AAIM_Note{ start, pitch, length } );
            }
            else if ( pitch == m_melody[ count ].getPitch() && trig == tie )
            {
                m_melody[ count ].setLength( m_melody[ count ].getLength() + 1 );
            }
            else
            {
                m_melody.emplace_back( AAIM_Note{ position, pitch, length } );
                count += 1;
            }
        }
        calculateAvailblePitches( m_melody );
    }
    
    void calculateAvailblePitches( std::vector< AAIM_Note > melody )
    {
//        auto pitchClassFlags  = std::array< bool, 12 >{};
//        m_availablePitchClasses.clear();
        auto len = melody.size();
        std::vector< int > pitches;
        pitches.reserve( len );
        for ( size_t i = 0; i < len; i++ )
        {
            if ( melody[ i ].getLength() > 0 )
            {
                pitches.emplace_back( melody[ i ].getPitch() );
            }
        }
        setAvailablePitchClasses( pitches );
    }
    

    
    size_t m_currentPitch, m_lastPitch;
    size_t m_nBeats = 8;
    std::vector< AAIM_Note > m_melody;
    std::vector< std::array< size_t, 2 > > m_melodyRaw;
    std::vector< int > m_availablePitchClasses;
};

#endif /* sjf_AAIM_melodyVary_h */

//
//  sjf_AAIM_patternVary.h
//  
//
//  Created by Simon Fay on 16/05/2023.
//

#ifndef sjf_AAIM_patternVary_h
#define sjf_AAIM_patternVary_h
#include "sjf_audio/sjf_audioUtilitiesC++.h"

template< class T >
class AAIM_patternVary
{
public:
    AAIM_patternVary()
    {
        setNumBeats( m_nBeats );
    }
    ~AAIM_patternVary(){}
    
    void setNumBeats( size_t nBeats )
    {
        nBeats = nBeats < 1 ? 1 : nBeats;
        if ( m_pattern.size() > nBeats )
            m_pattern.resize(nBeats);
        else
        {
            while( m_pattern.size() < nBeats )
                m_pattern.push_back( false );
        }
        m_nBeats = nBeats;
    }
    
    size_t getNumBeats()
    {
        return m_nBeats;
    }
    
    void setFills( T fills )
    {
        m_fills = std::fmin( std::fmax( fills, 0 ), 1 );
    }
    
    T getFills()
    {
        return m_fills;
    }
    
    void setBeat( size_t beatNumber, bool trueIfTriggerOnBeats )
    {
        if ( beatNumber < 0 || beatNumber >= m_pattern.size() )
        {
            return;
        }
        m_pattern[ beatNumber ] = trueIfTriggerOnBeats;
    }
    
    const std::vector< bool >& getPattern()
    {
        return m_pattern;
    }
    
    size_t getPatternLong()
    {
        auto outLong = 0ul;
        for ( size_t i = 0; i < m_pattern.size(); i++ )
        {
            outLong += m_pattern[ i ] ? (1<<i) : 0;
        }
        return outLong;
    }
    
    bool triggerBeat( T currentBeat, T beatsToSync = 1 )
    {
        // check distance from current position to nearest note in pattern
        auto distance = static_cast<T>(m_nBeats);
        for ( size_t i = 0; i < m_nBeats + 1; i++ ) // add one to total nBeats to circle to beginning of pattern at end
        {
            auto distFromPos = abs(currentBeat - i);
            distance = ( m_pattern[ i % m_nBeats ] && distFromPos < distance ) ? distFromPos : distance;
        }
        // if distance <= baseIOI && < random01
        // output trigger
        if ( distance < beatsToSync && (distance / beatsToSync) < rand01() )
            return true;
        // count the IOIs in pattern and divide by total number of beats
        auto count = 0;
        for ( size_t i = 0; i < m_nBeats; i++ )
            count = m_pattern[ i ] ? count + 1 : count;
        auto fract = static_cast<T>(count) / static_cast<T>(m_nBeats); // calculate fraction of pattern that has beats
        return ( fract*m_fills > rand01() ) ? true : false; // if ( fraction * fills probability ) is greater than a random number output a beat
    }
    
    bool getStep( size_t step )
    {
        return m_pattern[ step ];
    }
    
private:
    
    std::vector< bool > m_pattern;
    size_t m_nBeats = 8;
    T m_fills = 0;
};

#endif /* sjf_AAIM_patternVary_h */

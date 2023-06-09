//
//  sjf_AAIM_rhythmGen.h
//  
//
//  Created by Simon Fay on 13/05/2023.
//

#ifndef sjf_AAIM_rhythmGen_h
#define sjf_AAIM_rhythmGen_h


#include "sjf_audio/sjf_audioUtilitiesC++.h"

#define NUM_STORED_INDISPENSIBILITIES 2048
//============================================================
//============================================================
//============================================================
// This class generates the underlying rhythmic patterns used by the rhythm generator
// It creates a list with a value of how indispensabile each beat is
template < class T >
class AAIM_indispensability
{
public:
    static std::vector< size_t > generate23Grouping( size_t nBeatsToGenerate )
    {
        std::vector< size_t > grouping;
        while ( nBeatsToGenerate > 0 )
        {
            switch ( nBeatsToGenerate )
            {
                case 1:
                    grouping.push_back( 1 );
                    nBeatsToGenerate -= 1;
                    break;
                case 2:
                    grouping.push_back( 2 );
                    nBeatsToGenerate -= 2;
                    break;
                case 3:
                    grouping.push_back( 3 );
                    nBeatsToGenerate -= 3;
                    break;
                case 4:
                    grouping.push_back( 2 );
                    grouping.push_back( 2 );
                    nBeatsToGenerate -= 4;
                    break;
                default:
                    auto r = rand01() < 0.5 ? 2 : 3;
                    grouping.push_back( r );
                    nBeatsToGenerate -= r;
                    break;
            }
        }
        return grouping;
    }
    //============================================================
    static std::vector< T > generateindispensability( size_t nBeatsToGenerate )
    {
        if ( nBeatsToGenerate <= 1 )
            return std::vector< T >{ 1 };
        // really this should be recursive...
         // first calculate basic indispensability for total group
        auto grp = generate23Grouping( nBeatsToGenerate );
        std::vector< T > indis, indices;
        for ( size_t i = 0; i < nBeatsToGenerate; i++ )
        {
            indis.push_back(0);
            indices.push_back(i);
        }
        while ( nBeatsToGenerate > 1 )
        {
            auto grp = generate23Grouping( nBeatsToGenerate );
            auto newIndices = std::vector< T >{};
            auto count = 0;
            for ( size_t i = 0; i < grp.size(); i++ )
            {
                for ( size_t j = 0; j < grp[ i ]; j++ )
                {
                    auto indx = indices[ count ];
                    auto r = m_rules[ grp[i]-1 ][ j ];
                    if ( r == 1 )
                        newIndices.push_back( indx );
                    indis[ indx ] += r;
                    count++;
                }
            }
            indices = newIndices;
            nBeatsToGenerate = grp.size();
        }
        auto max = indis[0];
        for ( size_t i = 0; i < indis.size(); i++ )
            {
                indis[ i ] /= max; // scale to max of 1
                indis[ i ] *= 0.8; // then scale so values are between 0.2 and 0.8...
                indis[ i ] += 0.2;
            }
            
        return indis;
    }
private:
    static constexpr std::array< std::array < T, 3 >, 3 > m_rules
    {
        std::array < T, 3 >{1, 0, 0}, // group of 1
        std::array < T, 3 >{1, 0, 0}, // group of 2
        std::array < T, 3 >{1, 0, 0.5} // group of 3
    };
};
//============================================================
//============================================================
//============================================================
// This is the heart of the AAIM system
// it receives an audio rate ramp as input
template < class T >
class AAIM_rhythmGen
{
public:
    //============================================================
    AAIM_rhythmGen()
    {
        srand((unsigned)time(NULL));
        // initialise indispensability list
        for ( size_t i = 1; i <= NUM_STORED_INDISPENSIBILITIES; i++ )
            m_indispensabilityList.push_back( AAIM_indispensability< T >::generateindispensability( i ) );
        // default probability is max chance for the base ioi
        setIOIProbability( 1, 1 );
        setNumBeats( m_nBeats );
        chooseIOI( 0 ); // initialise
    };
    //============================================================
    ~AAIM_rhythmGen(){};
    //============================================================
    // this sets the global complexity value (i.e. chance of rhythmic divisions other than the base being chosen
    void setComplexity( T comp )
    {
        m_comp = std::fmin( std::fmax( comp, 0 ), 1 );
    }
    //============================================================
    // this gets the global complexity value (i.e. chance of rhythmic divisions other than the base being chosen
    T getComplexity( )
    {
        return m_comp;
    }
    //============================================================
    // this sets the global value for rests being inserted into the pattern
    void setRests( T rests )
    {
        m_rests = std::fmin( std::fmax( rests, 0 ), 1 );
    }
    //============================================================
    // this gets the global value for rests being inserted into the pattern
    T getRests( )
    {
        return m_rests;
    }
    //============================================================
    void setNumBeats( size_t nBeats )
    {
        m_nBeats = nBeats > 0 ? (nBeats < NUM_STORED_INDISPENSIBILITIES ? nBeats : NUM_STORED_INDISPENSIBILITIES ) : 1;
        m_baseindispensability = m_indispensabilityList[ nBeats-1 ];
    }
    //============================================================
    size_t getNumBeats( )
    {
        return m_nBeats;
    }
    //============================================================
    // this sets the probability of each rhythmic division being chosen
    //
    void setIOIProbability( T division, T chanceForThatDivision )
    {
        if ( division <= 0 )
            return;
        bool divIsInListFlag = false;
        for ( size_t i  = 0; i < m_IOIs.size(); i++ )
        {
            if ( m_IOIs[ i ].getDivsion() == division )
            {
                m_IOIs[ i ].setProbability( chanceForThatDivision );
                divIsInListFlag = true;
            }
        }
        if ( !divIsInListFlag )
            m_IOIs.push_back( AAIM_IOI{ division, chanceForThatDivision  } );
        m_limitedIOIs.reserve( m_IOIs.size() );
    }
    
    void setIOIList( std::vector< std::array < T, 2 > > ioiList )
    {
        if ( ioiList.size() < 1 )
            return;
        m_IOIs.clear();
        for ( size_t i = 0; i < ioiList.size(); i++ )
        {
            setIOIProbability( ioiList[ i ][ 0 ], ioiList[ i ][ 1 ] );
        }
        m_IOIs.shrink_to_fit();
    }
    //============================================================
    // This returns the division, probability, number of repetitions to sync, and number of base IOIs to sync
    const std::vector< std::array<T, 4> > getIOIProbabilities()
    {
        auto ioiSize = m_IOIs.size();
        std::vector< std::array<T, 4> > iois;
        iois.reserve(ioiSize);
        for ( size_t i = 0; i < ioiSize; i++ )
        {
            iois.emplace_back(
                              std::array<T, 4>
                              {
                                  m_IOIs[ i ].getDivsion(),
                                  m_IOIs[ i ].getProbability(),
                                  m_IOIs[ i ].getNumRepsToSync(),
                                  m_IOIs[ i ].getNumBaseIOIsToSync()
                              }
                              );
        }
        return iois;
    }
    //============================================================
    // this clears and resets the lists of probabilitites
    void clearIOIProbabilities()
    {
        m_clearFlag = true;
    }
    //============================================================
    // this should receive an input going from 0 --> nBeats ( 0-->1 is first beat, 1-->2 is second, etc )
    // output is phase through IOI, velocity determined by indispensability, whether a rest has been selected, the current division, the number of base IOIs that division takes to synchronise
    //..... possibly more info than needed but it seemed some of it could be useful
    std::array< T, 5 > runGenerator( T currentBeat )
    {
        // just to make sure the input isn't outside nBeats
        while( currentBeat >= m_nBeats )
            currentBeat -= m_nBeats;
        // calculate the phase within a single beat
        auto beat = static_cast<int>(currentBeat);
        auto baseIOIPhase = currentBeat - beat; // phase 0 --> 1
        // convert phase through baseIOI to phase through chosen ioi grouping
        // i.e. if an ioi of 0.75 is chosen it must repeat 4 times to sync to base ioi
        auto ioiPhase = m_phaseCount.rateChange( baseIOIPhase, m_IOIs[ m_currentIOI ].getNumBaseIOIsToSync() );
        ioiPhase *= m_IOIs[ m_currentIOI ].getInverseDivision(); // convert basePhaseCount to ioi ramp
        // compare random number to complexity value
        // output random ioi or baseIOI depending on result
        if (ioiPhase < 0.5 && m_lastIOIPhase >= 0.5)
        {
            m_currentIOI = rand01() <= m_comp ? chooseIOI( currentBeat ) : 0;
            m_vel = m_baseindispensability[ beat ] * m_IOIindispensability[ static_cast<int>(ioiPhase) ];
            m_restFlag = shouldRest( m_vel ) ? 0 : 1;
        }
        else if ( static_cast<int>(ioiPhase) > static_cast<int>(m_lastIOIPhase) )
        {
            m_vel = m_baseindispensability[ beat ] * m_IOIindispensability[ static_cast<int>(ioiPhase) ];
            m_restFlag = shouldRest( m_vel ) ? 0 : 1;
        }
        m_lastIOIPhase = ioiPhase; // save ioiPhase to use for comparison next time
        ioiPhase -= static_cast<int>(ioiPhase); // only output fractional part of ioi phase
        return { ioiPhase, m_vel, m_restFlag, m_IOIs[ m_currentIOI ].getDivsion(), m_IOIs[ m_currentIOI ].getNumBaseIOIsToSync() };
    }
    //============================================================
    std::vector< T > getBaseindispensability()
    {
        return m_baseindispensability;
    }
    //============================================================
    std::vector< T > getIOIindispensability()
    {
        return m_IOIindispensability;
    }
    //============================================================
private:
    //============================================================
    // this chooses from the list of possible IOIs
    // choices are limited by the number of beats remaining
    // choices are further weighted by whether or not they syncronise to the underlying grouping
    size_t chooseIOI( T currentBeat )
    {
        if ( m_clearFlag )
        {
            m_IOIs.clear();
            setIOIProbability( 1, 1 );
            m_IOIs.shrink_to_fit();
            m_clearFlag = false;
        }
        // first just ensure we are within the number of beats
        while ( currentBeat >= m_nBeats )
            currentBeat -= m_nBeats;
        auto nBeatsLeft = m_nBeats - currentBeat;
//        auto nIOIs = m_IOIs.size();
        auto nPossibleIOIs = 0ul;
        m_limitedIOIs.clear();
        for ( size_t i = 0; i < m_IOIs.size(); i++ )
        {
            auto prob = m_IOIs[ i ].getProbability();
            auto baseIOIsToSync = m_IOIs[ i ].getNumBaseIOIsToSync();
            // only include IOIs that fit within the remaining beats and have a probability > 0
            if ( ( baseIOIsToSync <= nBeatsLeft ) && ( prob > 0.0f ) )
            {
                // scale probability for possible IOIs according to whether they sync with local maxima in the indispensability list
                auto indexForSynchronisation = (size_t)currentBeat + (size_t)baseIOIsToSync;
                auto indexIndis = m_baseindispensability[ indexForSynchronisation ];
                auto preIndis = m_baseindispensability[ indexForSynchronisation - 1 ];
                auto postIndis = m_baseindispensability[ ((indexForSynchronisation + 1) % (size_t)m_nBeats) ];
                if ( indexIndis > preIndis && indexIndis > postIndis )
                    prob *= 2.0f;
                m_limitedIOIs.emplace_back( AAIM_PossibleIOI{ prob, i } );
                nPossibleIOIs++;
            }
        }
        
        auto total = 0.0f; // this store the total of all of the "probabilities" set
        for ( size_t i = 0; i < nPossibleIOIs; i++ )
        {
            total += m_limitedIOIs[ i ].getProbability();
            // scale probabilities from set values to a scale from 0 --> total
            if ( i != 0 )
                m_limitedIOIs[ i ].setProbability( m_limitedIOIs[ i ].getProbability() + m_limitedIOIs[ i-1 ].getProbability() );
        }
        auto r = rand01() * total;
        auto chosenIOI = 0ul;
        for ( size_t i = 0; i < m_IOIs.size(); i++ )
        {
            if ( m_IOIs[ i ].getDivsion() == 1.0 )
            {
                chosenIOI = i;
                break;
            }
        }
        auto ioiChosen = false;
        for ( size_t i = 0; i < nPossibleIOIs; i++ )
        {
            if ( r < m_limitedIOIs[ i ].getProbability() )
            {
                chosenIOI = m_limitedIOIs[ i ].getOriginalIndex();
                ioiChosen = true;
            }
            if (ioiChosen)
                break;
        }
        m_IOIindispensability = m_indispensabilityList[ m_IOIs[ chosenIOI ].getNumRepsToSync() ];
        return chosenIOI;
    }
    //============================================================
    // this determines whether a specific beat should have a reverse
    // steps velocity already incorporates both step through base ioi and current ioi
    bool shouldRest( T velocity )
    {
        return rand01() < m_rests * ( 1.0f - velocity ) ? true : false;
    }
    
    //============================================================
    //============================================================
    //============================================================
    // This is just a simple class to add a counter to the input phase
    class phaseCounter
    {
    public:
        phaseCounter(){};
        ~phaseCounter(){};
        T rateChange( const T phaseIn, const size_t nBeats )
        {
            if ( phaseIn < (m_lastPhase * 0.5) )
            {
                m_count += 1;
                while ( m_count >= nBeats )
                    m_count -= nBeats;
            }
            m_lastPhase = phaseIn;
            return static_cast<T>(m_count) + m_lastPhase;
        }
        void reset()
        {
            m_count = 0;
            m_lastPhase = 0;
        }
    private:
        T m_count = 0, m_lastPhase = 0;
    };
    //============================================================
    //============================================================
    // This is a simple class to hold the relative information for each IOI
    // originally I used an array for this but I thought
    //      this might make  be a little clearer
    class AAIM_IOI
    {
    public:
        AAIM_IOI( T division, T probability )
            :   m_division{ division },
                m_probability{ static_cast<T>(std::fmin( std::fmax( 0, probability ), 1 ) ) },
                m_invDivision{ 1.0f / division }
        {
            calculateNumBeatsToSync();
        }
        ~AAIM_IOI(){}
        T getDivsion(){ return m_division; }
        T getProbability(){ return m_probability; }
        void setProbability( T prob )
        {
            // ensure probability is between 0 and 1
            m_probability = std::fmin( std::fmax( 0, prob ), 1 );
        }
        T getInverseDivision(){ return m_invDivision; }
        T getNumRepsToSync(){ return m_nRepsToSync; }
        T getNumBaseIOIsToSync(){ return m_nBaseIOIsToSync; }
    private:
        void calculateNumBeatsToSync()
        {
            auto eps = m_division * 0.001f; // arbitrarily small difference to compare to
            // calculate how many times the ioi in question needs to repeat before it synchronises
            // no guarantee this won't go forever :/
            // need to think of a solution...
            auto reps = 1.0f;
            while( ( ( m_division * reps) - static_cast<int>(m_division * reps) ) > eps )
                reps += 1;
            m_nRepsToSync = reps;
            m_nBaseIOIsToSync = (m_nRepsToSync * m_division );
        }
        
        T m_division {}, m_probability {}, m_invDivision {};
        T m_nRepsToSync = 0, m_nBaseIOIsToSync = 0;
    };
    //============================================================
    //============================================================
    // This is a simple class to hold information about the IOIs that
    //      are possible after limiting accoring to nBeats left in pattern
    class AAIM_PossibleIOI
    {
    public:
        AAIM_PossibleIOI ( T prob, size_t originalIndex ) :
            m_probability{ prob },
            m_originalIndex{ originalIndex }
        {}
        ~AAIM_PossibleIOI(){}
        T getProbability(){ return m_probability; }
        void setProbability( T prob ) { m_probability = prob; }
        size_t getOriginalIndex(){ return m_originalIndex; }
    private:
        T m_probability{};
        size_t m_originalIndex{};
    };
    //============================================================
    //============================================================
    T m_comp = 0, m_rests = 0, m_nBeats = 8, m_lastIOIPhase = 1, m_vel = 1, m_restFlag = 1;
    size_t m_currentIOI = 0;
    std::vector< AAIM_IOI > m_IOIs;
    std::vector< AAIM_PossibleIOI > m_limitedIOIs;
    std::vector< T > m_baseindispensability, m_IOIindispensability;
    phaseCounter m_phaseCount;
    std::vector< std::vector< T > > m_indispensabilityList;
    bool m_clearFlag = false;
};




#endif /* sjf_AAIM_rhythmGen_h */

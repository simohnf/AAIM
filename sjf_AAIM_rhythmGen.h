//
//  sjf_AAIM_rhythmGen.h
//  
//
//  Created by Simon Fay on 13/05/2023.
//

#ifndef sjf_AAIM_rhythmGen_h
#define sjf_AAIM_rhythmGen_h


#include "sjf_audio/sjf_audioUtilitiesC++.h"

#define NUM_STORED_INDISPENSIBILITIES 1024
//============================================================
//============================================================
//============================================================
// This class generates the underlying rhythmic patterns used by the rhythm generator
// It creates a list with a value of how indispensabile each beat is
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
    static std::vector< float > generateindispensability( size_t nBeatsToGenerate )
    {
        if ( nBeatsToGenerate <= 1 )
            return std::vector< float >{ 1 };
        // really this should be recursive...
         // first calculate basic indispensability for total group
        auto grp = generate23Grouping( nBeatsToGenerate );
        std::vector< float > indis, indices;
        for ( size_t i = 0; i < nBeatsToGenerate; i++ )
        {
            indis.push_back(0);
            indices.push_back(i);
        }
        while ( nBeatsToGenerate > 1 )
        {
            auto grp = generate23Grouping( nBeatsToGenerate );
            auto newIndices = std::vector< float >{};
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
    static constexpr std::array< std::array < float, 3 >, 3 > m_rules
    {
        std::array < float, 3 >{1, 0, 0}, // group of 1
        std::array < float, 3 >{1, 0, 0}, // group of 2
        std::array < float, 3 >{1, 0, 0.5} // group of 3
    };
};
//============================================================
//============================================================
//============================================================
// This is the heart of the AAIM system
// it receives an audio rate ramp as input
class AAIM_rhythmGen
{
public:
    //============================================================
    AAIM_rhythmGen()
    {
        srand((unsigned)time(NULL));
        // initialise indispensability list
        for ( size_t i = 1; i <= NUM_STORED_INDISPENSIBILITIES; i++ )
            m_indispensabilityList.push_back( AAIM_indispensability::generateindispensability( i ) );
        // default probability is max chance for the base ioi
        setIOIProbability( 1, 1 );
        setNumBeats( m_nBeats );
        chooseIOI( 0 ); // initialise
        

    };
    //============================================================
    ~AAIM_rhythmGen(){};
    //============================================================
    // this sets the global complexity value (i.e. chance of rhythmic divisions other than the base being chosen
    void setComplexity( float comp )
    {
        m_comp = std::fmin( std::fmax( comp, 0 ), 1 );
    }
    //============================================================
    // this gets the global complexity value (i.e. chance of rhythmic divisions other than the base being chosen
    float getComplexity( )
    {
        return m_comp;
    }
    //============================================================
    // this sets the global value for rests being inserted into the pattern
    void setRests( float rests )
    {
        m_rests = std::fmin( std::fmax( rests, 0 ), 1 );
    }
    //============================================================
    // this gets the global value for rests being inserted into the pattern
    float getRests( )
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
    void setIOIProbability( float division, float chanceForThatDivision )
    {
        if ( division == 0 )
            return;
        bool divIsInListFlag = false;
        for ( size_t i  = 0; i < m_probs.size(); i++ )
        {
            if ( m_probs[ i ][ 0 ] == division )
            {
                m_probs[ i ][ 1 ] = chanceForThatDivision;
                divIsInListFlag = true;
            }
        }
        if ( !divIsInListFlag )
        {
            auto newDiv = std::array<float, 3>{ division, chanceForThatDivision, 1.0f/division  };
            m_probs.push_back( newDiv );
        }
        calculateNumBeatsToSync();
        m_limitedProbs.reserve( m_probs.size() );
    }
    
    //============================================================
    const std::vector< std::array<float, 4> > getIOIProbabilities()
    {
        auto ioiSize = m_probs.size();
        std::vector< std::array<float, 4> > iois;
        iois.resize(ioiSize);
        for ( size_t i = 0; i < ioiSize; i++ )
        {
            iois[ i ][ 0 ] = m_probs[ i ][ 0 ];
            iois[ i ][ 1 ] = m_probs[ i ][ 1 ];
            iois[ i ][ 2 ] = m_nRepsToSync[ i ];
            iois[ i ][ 3 ] = m_nBaseIOIsToSync[ i ];
        }
        return iois;
    }
    
    //============================================================
    // this should receive an input going from 0 --> nBeats ( 0-->1 is first beat, 1-->2 is second, etc )
    // output is phase through IOI and velocity determined by indispensability
    std::vector< float > runGenerator( float currentBeat )
    {
        // just to make sure the input isn't outside nBeats
        while( currentBeat >= m_nBeats )
            currentBeat -= m_nBeats;
        // calculate the phase within a single beat
        auto beat = (int)currentBeat;
        auto baseIOIPhase = currentBeat - beat; // phase 0 --> 1
        // convert phase through baseIOI to phase through chosen ioi grouping
        // i.e. if an ioi of 0.75 is chosen it must repeat 4 times to sync to base ioi
        auto ioiPhase = m_phaseCount.rateChange( baseIOIPhase, m_nBaseIOIsToSync[ m_currentIOI ] );
        ioiPhase *= m_probs[ m_currentIOI ][ 2 ]; // convert basePhaseCount to ioi ramp
        // compare random number to complexity value
        // output random ioi or baseIOI depending on result
        if (ioiPhase < 0.5 && m_lastIOIPhase >= 0.5)
        {
            m_currentIOI = rand01() <= m_comp ? chooseIOI( currentBeat ) : 0;
            m_vel = m_baseindispensability[ beat ] * m_ioiindispensability[ (int)ioiPhase ];
            m_restFlag = shouldRest( m_vel ) ? 0 : 1;
//            m_restFlag = rand01() < m_rests ? 1 : 0;
        }
        else if ( (int)ioiPhase > (int)m_lastIOIPhase )
        {
            m_vel = m_baseindispensability[ beat ] * m_ioiindispensability[ (int)ioiPhase ];
            m_restFlag = shouldRest( m_vel ) ? 0 : 1;
//            m_restFlag = rand01() < m_rests ? 1 : 0;
        }
        m_lastIOIPhase = ioiPhase; // save ioiPhase to use for comparison next time
        ioiPhase -= (int)ioiPhase; // only output fractional part of ioi phase
        return { ioiPhase, m_vel, m_restFlag };
    }
    //============================================================
    std::vector< float > getBaseindispensability()
    {
        return m_baseindispensability;
    }
    //============================================================
    std::vector< float > getIOIindispensability()
    {
        return m_ioiindispensability;
    }
    //============================================================
private:
    //============================================================
    void calculateNumBeatsToSync()
    {
        //        auto eps = std::numeric_limits<float>::epsilon(); // smallest value possible
        auto nIOIs = m_probs.size();
        if ( nIOIs < m_nRepsToSync.size() )
        {
            // nIOIs has shrunk reset and start again
            m_nRepsToSync.resize(0);
            m_nBaseIOIsToSync.resize(0);
        }
        for ( size_t i = m_nRepsToSync.size(); i < nIOIs; i++ )
        {
            auto eps = (m_probs[ i ][ 0 ] * 0.001f); // arbitrarily small difference to compare to
            // calculate how many times the ioi in question needs to repeat before it synchronises
            // no guarantee this won't go forever :/
            // need to think of a solution...
            auto reps = 1.0f;
            while( ( (m_probs[ i ][ 0 ] * reps) - (int)(m_probs[ i ][ 0 ] * reps) ) > eps )
                reps += 1;
            m_nRepsToSync.push_back( reps );
            m_nBaseIOIsToSync.push_back( m_nRepsToSync[ i ] * m_probs[ i ][ 0 ] );
        }
    }
    //============================================================
    // this chooses from the list of possible IOIs
    // choices are limited by the number of beats remaining
    // choices are further weighted by whether or not they syncronise to the underlying grouping
    size_t chooseIOI( float currentBeat )
    {
        // first just ensure we are within the number of beats
        while ( currentBeat >= m_nBeats )
            currentBeat -= m_nBeats;
        auto nBeatsLeft = m_nBeats - currentBeat;
        auto nIOIs = m_probs.size();
        auto count = 0;
        for ( size_t i = 0; i < nIOIs; i++ )
        {
            if ( m_nBaseIOIsToSync[ i ] <= nBeatsLeft )
            {
                for ( size_t j = 0; j < m_probs[0].size(); j++ )
                    {
                        m_limitedProbs[count][ j ] = m_probs[ i ][ j ];
                    }
                // scale probabilities for possible IOIs according to whether they sync with local maxima in the indispensability list
                auto indexForSynchronisation = (size_t)currentBeat + (size_t)m_nBaseIOIsToSync[ i ];
                auto indexIndis = m_baseindispensability[ indexForSynchronisation ];
                auto preIndis = m_baseindispensability[ indexForSynchronisation - 1 ];
                auto postIndis = m_baseindispensability[ ((indexForSynchronisation + 1) % (size_t)m_nBeats) ];
                if ( indexIndis < preIndis || indexIndis < postIndis )
                    m_limitedProbs[count][ 1 ] *= 0.5;
                count++;
            }
        }
        nIOIs = count; // reset to the number of possible IOIs given the beats remaining
        
        auto total = 0.0f; // this store the total of all of the "probabilities" set
        for ( size_t i = 0; i < nIOIs; i++ )
        {
            total += m_limitedProbs[ i ][ 1 ];
            // scale probabilities from set values to a scale from 0 --> total
            m_limitedProbs[ i ][ 1 ] = (i != 0) ? m_limitedProbs[ i ][ 1 ] + m_limitedProbs[ i-1 ][ 1 ] : m_limitedProbs[ i ][ 1 ];
        }
        auto r = rand01() * total;
        auto chosenIOI = 0ul;
        auto ioiChosen = false;
        for ( size_t i = 0; i < nIOIs; i++ )
        {
            if ( r <= m_limitedProbs[ i ][ 1 ] )
            {
                chosenIOI = i;
                ioiChosen = true;
            }
            if (ioiChosen)
                break;
        }
        m_ioiindispensability = m_indispensabilityList[ m_nRepsToSync[ chosenIOI ] ];
        return chosenIOI;
    }
    //============================================================
    // this determines whether a specific beat should have a reverse
    // steps velocity already incorporates both step through base ioi and current ioi
    bool shouldRest( float velocity )
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
        float rateChange( const float phaseIn, const size_t nBeats )
        {
            if ( phaseIn < (m_lastPhase * 0.5) )
            {
                m_count += 1;
                while ( m_count >= nBeats )
                    m_count -= nBeats;
            }
            m_lastPhase = phaseIn;
            return (float)m_count + m_lastPhase;
        }
        void reset()
        {
            m_count = 0;
            m_lastPhase = 0;
        }
    private:
        float m_count = 0, m_lastPhase = 0;
    };
    //============================================================
    float m_comp = 0, m_rests = 0, m_nBeats = 8, m_lastIOIPhase = 1, m_vel = 1, m_restFlag = 1;
    size_t m_currentIOI = 0;
    std::vector< std::array<float, 3> > m_probs, m_limitedProbs;
    std::vector< float > m_nRepsToSync, m_nBaseIOIsToSync;
    std::vector< float > m_baseindispensability, m_ioiindispensability;
    phaseCounter m_phaseCount;
    std::vector< std::vector< float > > m_indispensabilityList;
};




#endif /* sjf_AAIM_rhythmGen_h */

#pragma once
#include <arbiter/details/ArbiterCacheAdvancerContext.hpp>
#include <cstddef>

namespace arbiter { namespace details {

    template<class Traits>
    class AdvanceHead
    {
    public:
        using ErrorReportingPolicy = typename Traits::ErrorReportingPolicy;
        using SequenceType = typename Traits::SequenceType;
        using SeqInfo = SequenceInfo<SequenceType, Traits::NumberOfLines()>;

        bool advance(ArbiterCacheAdvancerContext<Traits>& context, const std::size_t lineId, const SequenceType sequenceNumber);

    private:
        void checkForSlowLineOverrun(ArbiterCacheAdvancerContext<Traits>& context, const std::size_t lineId, const std::size_t nextPosition);
        void handleGaps(const SeqInfo& sequenceInfo, ErrorReportingPolicy& errorPolicy);
    };


    template<class Traits>
    bool AdvanceHead<Traits>::advance(ArbiterCacheAdvancerContext<Traits>& context, const std::size_t lineId, const SequenceType sequenceNumber)
    {
        auto& cache = context.cache;

        auto nextPosition = cache.nextPosition(lineId);
        auto& sequenceInfo = cache.history[nextPosition];

        checkForSlowLineOverrun(context, lineId, nextPosition);
        handleGaps(sequenceInfo, context.errorPolicy);

        cache.history[nextPosition] = SeqInfo(lineId, sequenceNumber);
        cache.positions[lineId] = nextPosition;

        return true;    // new sequence number, accept the message
    }

    template<class Traits>
    void AdvanceHead<Traits>::checkForSlowLineOverrun(ArbiterCacheAdvancerContext<Traits>& context, const std::size_t lineId, const std::size_t nextPosition)
    {
        auto& positions = context.cache.positions;

        std::size_t positionLineId = 0;
        for(auto& position : positions)
        {
            if(positionLineId != lineId)
            {
                if(nextPosition == position)
                {
                    context.errorPolicy.LinePositionOverrun(positionLineId, lineId);
                    position = (nextPosition + 1) % context.cache.history.size();
                }
            }

            ++positionLineId;
        }
    }

    template<class Traits>
    void AdvanceHead<Traits>::handleGaps(const SeqInfo& sequenceInfo, ErrorReportingPolicy& errorPolicy)
    {
        if(!sequenceInfo.complete())
        {
            if(sequenceInfo.empty())
            {
                errorPolicy.UnrecoverableGap(sequenceInfo.sequence());
            }
            else
            {
                const auto& missingLines = sequenceInfo.lines().missing();
                for(auto line : missingLines)
                {
                    errorPolicy.UnrecoverableLineGap(line, sequenceInfo.sequence());
                }
            }
        }
    }
}}

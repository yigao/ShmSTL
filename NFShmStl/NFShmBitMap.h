// -------------------------------------------------------------------------
//    @FileName         :    NFShmBitMap.h
//    @Author           :    gaoyi
//    @Date             :    24-9-10
//    @Email            :    445267987@qq.com
//    @Module           :    NFShmBitMap
//
// -------------------------------------------------------------------------

#pragma once

#include "NFShmStl.h"
#include "NFShmBitSet.h"

template<int MaxSize>
class NFShmBitMap : public NFShmBitSet<MaxSize>
{
public:
    NFShmBitMap()
    {
        if (SHM_CREATE_MODE)
        {
            CreateInit();
        }
        else
        {
            ResumeInit();
        }
    }

    int CreateInit()
    {
        return 0;
    }

    int ResumeInit()
    {
        return 0;
    }

    ~NFShmBitMap()
    {
    }

    NFShmBitMap<MaxSize> &operator =(NFShmBitMap<MaxSize> &rExBitMap);

    bool IsBitSetted(int iBitSeq) const
    {
        CHECK_EXPR(iBitSeq < MaxSize, false, "out of range bitset, TRACE_STACK:%s", TRACE_STACK());
        return NFShmBitSet<MaxSize>::test(iBitSeq);
    }

    int SetBit(int iBitSeq)
    {
        CHECK_EXPR(iBitSeq < MaxSize, -1, "out of range bitset, TRACE_STACK:%s", TRACE_STACK());
        NFShmBitSet<MaxSize>::set(iBitSeq, true);
        return 0;
    }

    int ClearBit(int iBitSeq)
    {
        CHECK_EXPR(iBitSeq < MaxSize, -1, "out of range bitset, TRACE_STACK:%s", TRACE_STACK());
        NFShmBitSet<MaxSize>::set(iBitSeq, false);
        return 0;
    }

    int ClearAllBits()
    {
        NFShmBitSet<MaxSize>::reset();
        return 0;
    }

    std::string Get() const
    {
        return NFShmBitSet<MaxSize>::to_string();
    }


    void Get(std::string& str) const
    {
        str = NFShmBitSet<MaxSize>::to_string();
    }

    int Set(const std::string& str)
    {
        NFShmBitSet<MaxSize>::_M_copy_from_string(str, 0, str.size());
        return 0;
    }
};

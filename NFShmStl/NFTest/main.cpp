// -------------------------------------------------------------------------
//    @FileName         :    main.cpp
//    @Author           :    gaoyi
//    @Date             :    23-8-19
//    @Email			:    445267987@qq.com
//    @Module           :    main
//
// -------------------------------------------------------------------------

#include "Common.h"

#include <type_traits>
#include <string>
#include <NFComm/NFPluginModule/NFCheck.h>
#include <NFComm/NFPluginModule/NFStackTrace.h>
#include "NFComm/NFShmStl/NFShmStl.h"
#include "TestVector.h"
#include "TestList.h"
#include "TestRBTree.h"
#include "TestNFShmRBTreeWithList.h"
#include "TestNFShmMap.h"
#include "TestNFShmMultiMap.h"
#include "TestNFShmSet.h"
#include "TestNFShmMultiSet.h"
#include "TestNFShmHashTable.h"
#include "TestNFShmHashTableAdvanced.h"
#include "TestNFShmHashTableErase.h"
#include "TestNFShmHashTableEraseAdvanced.h"
// 新增哈希容器测试
#include "TestNFShmHashMap.h"
#include "TestNFShmHashSet.h"
#include "TestNFShmHashMultiMap.h"
#include "TestNFShmHashMultiSet.h"
#include "TestNFShmHashTableWithList.h"

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}



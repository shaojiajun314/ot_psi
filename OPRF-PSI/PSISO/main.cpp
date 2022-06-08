#include "PSI/include/Defines.h"
#include "PSI/include/utils.h"
#include "PSI/include/PsiSender.h"
#include "PSI/include/PsiReceiver.h"
#include "PSI/include/PsiSenderForTest.h"
#include "PSI/include/PsiReceiverForTest.h"

#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Network/Endpoint.h>
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Common/CLP.h>
#include <cryptoTools/Common/Log.h>

#include <vector>

using namespace std;
using namespace PSI;

const block commonSeed = oc::toBlock(123456);

u64 senderSize;
u64 receiverSize;
u64 width;
u64 height;
u64 logHeight;
u64 hashLengthInBytes;
char *ip;

extern "C"
void runSender(
	u64 senderSize,
	u64 receiverSize,
	u64 width,
	u64 height,
	u64 logHeight,
	u64 hashLengthInBytes,
	char * ip,
	unsigned long long RawDataSet[][2]
) {
	u64 bucket, bucket1, bucket2;
	bucket1 = bucket2 = 1 << 8;
	IOService ios;
	string ip_string = ip;
	Endpoint ep(ios, ip_string, EpMode::Server, "test-psi");
	Channel ch = ep.addChannel();
	vector<block> senderSet(senderSize);
	PRNG prng(oc::toBlock(123));
	for (auto i = 0; i < senderSize; ++i) {
		senderSet[i] = _mm_set_epi64x(RawDataSet[i][0], RawDataSet[i][1]);
	}
	PsiSender psiSender;
	psiSender.run(prng, ch, commonSeed, senderSize, receiverSize, height, logHeight, width, senderSet, hashLengthInBytes, 32, bucket1, bucket2);
	ch.close();
	ep.stop();
	ios.stop();
}

extern "C"
PSIRet  runReceiver(
	u64 senderSize,
	u64 receiverSize,
	u64 width,
	u64 height,
	u64 logHeight,
	u64 hashLengthInBytes,
	char *ip,
	unsigned long long RawDataSet[][2]
) {
	u64 bucket, bucket1, bucket2;
	bucket1 = bucket2 = 1 << 8;
	IOService ios;
	string ip_string = ip;
	Endpoint ep(ios, ip_string, EpMode::Client, "test-psi");
	Channel ch = ep.addChannel();

	vector<block> receiverSet(receiverSize);
	PRNG prng(oc::toBlock(123));
	for (auto i = 0; i < receiverSize; ++i) {
		receiverSet[i] = _mm_set_epi64x(RawDataSet[i][0], RawDataSet[i][1]);
	}
	PsiReceiver psiReceiver;
	PSIRet psiRet;
	psiRet = psiReceiver.run(prng, ch, commonSeed, senderSize, receiverSize, height, logHeight, width, receiverSet, hashLengthInBytes, 32, bucket1, bucket2);
	std::cout << psiRet.PSISet << " psiRet.PSISet addr \n";
	ch.close();
	ep.stop();
	ios.stop();
	return psiRet;
}

extern "C"
void runSenderForTest(
	u64 senderSize,
	u64 receiverSize,
	u64 width,
	u64 height,
	u64 logHeight,
	u64 hashLengthInBytes,
	char * ip,
	unsigned long long RawDataSet[][2]
) {
	u64 bucket, bucket1, bucket2;
	bucket1 = bucket2 = 1 << 8;
	IOService ios;
	string ip_string = ip;
	Endpoint ep(ios, ip_string, EpMode::Server, "test-psi");
	Channel ch = ep.addChannel();
	vector<block> senderSet(senderSize);
	PRNG prng(oc::toBlock(123));
	for (auto i = 0; i < senderSize; ++i) {
		senderSet[i] = _mm_set_epi64x(RawDataSet[i][0], RawDataSet[i][1]);
	}
	PsiSenderForTest psiSender;
	psiSender.run(prng, ch, commonSeed, senderSize, receiverSize, height, logHeight, width, senderSet, hashLengthInBytes, 32, bucket1, bucket2);
	ch.close();
	ep.stop();
	ios.stop();
}

extern "C"
PSIRet  runReceiverForTest(
	u64 senderSize,
	u64 receiverSize,
	u64 width,
	u64 height,
	u64 logHeight,
	u64 hashLengthInBytes,
	char *ip,
	unsigned long long RawDataSet[][2]
) {
	u64 bucket, bucket1, bucket2;
	bucket1 = bucket2 = 1 << 8;
	IOService ios;
	string ip_string = ip;
	Endpoint ep(ios, ip_string, EpMode::Client, "test-psi");
	Channel ch = ep.addChannel();

	vector<block> receiverSet(receiverSize);
	PRNG prng(oc::toBlock(123));
	for (auto i = 0; i < receiverSize; ++i) {
		receiverSet[i] = _mm_set_epi64x(RawDataSet[i][0], RawDataSet[i][1]);
	}
	PsiReceiverForTest psiReceiver;
	PSIRet psiRet;
	psiRet = psiReceiver.run(prng, ch, commonSeed, senderSize, receiverSize, height, logHeight, width, receiverSet, hashLengthInBytes, 32, bucket1, bucket2);
	std::cout << psiRet.PSISet << " psiRet.PSISet addr \n";
	ch.close();
	ep.stop();
	ios.stop();
	return psiRet;
}


extern "C"
void free_heap(unsigned long long* retArray){
	std::cout << retArray << " psiRet.PSISet addr2 \n";
	free(retArray);
	std::cout << " ok \n";
}

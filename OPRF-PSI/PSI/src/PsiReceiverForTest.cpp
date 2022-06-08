#include "PsiReceiverForTest.h"

#include <array>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <unordered_map>
#include <iomanip>
#include <bitset>
#include <thread>
#include <ctime>

#include<stdio.h>
#include <unistd.h>
#include<fcntl.h>
#include<fstream>
#include<string>


namespace PSI {
	//输出u8数据
	void PrintReceiverU8Data(u8* data, char* name, int length)
	{
        block* tmp = (block*)(data);
		time_t now = time(nullptr);
		char* curr_time = ctime(&now);
		std::cout << curr_time;
		std::cout << "name:" << name << '\n';
        for (int i=0; i<length; i++)
		{
				printf("%d",data[i]);
				// printf("%c", data[i]);
        }
		std::cout << "\n";
    }

	//输出block数据
	void PrintReceiverBlockData(block* data, int number,char* name)
	{
		time_t now = time(nullptr);
		char* curr_time = ctime(&now);
		std::cout << curr_time;
		std::cout << "name:" << name << " number:" << number << '\n';
		for (int i = 0; i < number; ++i) {
			std::cout << data[i] << '\n';
		}
    }


	PSIRet PsiReceiverForTest::run(PRNG& prng, Channel& ch, block commonSeed, const u64& senderSize, const u64& receiverSize, const u64& height, const u64& logHeight, const u64& width, std::vector<block>& receiverSet, const u64& hashLengthInBytes, const u64& h1LengthInBytes, const u64& bucket1, const u64& bucket2) {

		int sender2recieverfd;
		int reciever2senderfd;
		// sender2recieverfd = open("t_sender2receiver", O_RDONLY);
		// reciever2senderfd = open("t_receiver2sender", O_WRONLY);
		sender2recieverfd = open("r_sender2receiver", O_RDONLY);
		reciever2senderfd = open("r_receiver2sender", O_WRONLY);

		Timer timer;

		timer.setTimePoint("Receiver start");

		TimeUnit start, end;

		auto heightInBytes = (height + 7) / 8;
		auto widthInBytes = (width + 7) / 8;
		auto locationInBytes = (logHeight + 7) / 8;
		auto receiverSizeInBytes = (receiverSize + 7) / 8;
		auto shift = (1 << logHeight) - 1;
		auto widthBucket1 = sizeof(block) / locationInBytes;


		///////////////////// Base OTs ///////////////////////////

		IknpOtExtSender otExtSender;
		otExtSender.genBaseOts(prng, ch);

		std::vector<std::array<block, 2> > otMessages(width);

		otExtSender.send(otMessages, prng, ch);

		// std::cout << "Receiver base OT finished\n";
		timer.setTimePoint("Receiver base OT finished");




		//////////// Initialization ///////////////////

		PRNG commonPrng(commonSeed);
		block commonKey;
		AES commonAes;

		u8* matrixA[widthBucket1];
		u8* matrixDelta[widthBucket1];
		for (auto i = 0; i < widthBucket1; ++i) {
			matrixA[i] = new u8[heightInBytes];
			matrixDelta[i] = new u8[heightInBytes];
		}

		u8* transLocations[widthBucket1];
		for (auto i = 0; i < widthBucket1; ++i) {
			transLocations[i] = new u8[receiverSize * locationInBytes + sizeof(u32)];
		}

		block randomLocations[bucket1];


		u8* transHashInputs[width];
		for (auto i = 0; i < width; ++i) {
			transHashInputs[i] = new u8[receiverSizeInBytes];
			memset(transHashInputs[i], 0, receiverSizeInBytes);
		}

		// std::cout << "Receiver initialized\n";
		timer.setTimePoint("Receiver initialized");




		/////////// Transform input /////////////////////

		commonPrng.get((u8*)&commonKey, sizeof(block));
		commonAes.setKey(commonKey);

		block* recvSet = new block[receiverSize];
		block* aesInput = new block[receiverSize];
		block* aesOutput = new block[receiverSize];

		RandomOracle H1(h1LengthInBytes);
		u8 h1Output[h1LengthInBytes];

		for (auto i = 0; i < receiverSize; ++i) {
			H1.Reset();
			H1.Update((u8*)(receiverSet.data() + i), sizeof(block));
			H1.Final(h1Output);

			aesInput[i] = *(block*)h1Output;
			recvSet[i] = *(block*)(h1Output + sizeof(block));
		}

		commonAes.ecbEncBlocks(aesInput, receiverSize, aesOutput);
		for (auto i = 0; i < receiverSize; ++i) {
			recvSet[i] ^= aesOutput[i];
		}

		// std::cout << "Receiver set transformed\n";
		//把元素集合中每一个元素y的v_y输出, 第i行就是第i个元素的v_y=F_k(H_1(y))
		PrintReceiverBlockData(recvSet, receiverSize, "v_y");
		std::cout << "For element y in data set Y, the receiver uses AES to calculate the v_y=F_k(H_1(y)) function\n";
		timer.setTimePoint("Receiver set transformed");





		for (auto wLeft = 0; wLeft < width; wLeft += widthBucket1) {
			auto wRight = wLeft + widthBucket1 < width ? wLeft + widthBucket1 : width;
			auto w = wRight - wLeft;


			//////////// Compute random locations (transposed) ////////////////

			commonPrng.get((u8*)&commonKey, sizeof(block));
			commonAes.setKey(commonKey);

			for (auto low = 0; low < receiverSize; low += bucket1) {

				auto up = low + bucket1 < receiverSize ? low + bucket1 : receiverSize;

				commonAes.ecbEncBlocks(recvSet + low, up - low, randomLocations);

				for (auto i = 0; i < w; ++i) {
					for (auto j = low; j < up; ++j) {
						memcpy(transLocations[i] + j * locationInBytes, (u8*)(randomLocations + (j - low)) + i * locationInBytes, locationInBytes);
						//把元素集合中每一个元素y对应的多个染色位置输出
						PrintReceiverU8Data((u8*)(randomLocations + (j - low)) + i * locationInBytes, "v_y[i]", locationInBytes);
					}
					//不同元素之间使用换行标识
					std::cout << "\n";
				}
			}
			std::cout << "Record the value of v_y[i]\n";



			//////////// Compute matrix Delta /////////////////////////////////

			for (auto i = 0; i < widthBucket1; ++i) {
				memset(matrixDelta[i], 255, heightInBytes);
			}

			for (auto i = 0; i < w; ++i) {
				for (auto j = 0; j < receiverSize; ++j) {
					auto location = (*(u32*)(transLocations[i] + j * locationInBytes)) & shift;

					matrixDelta[i][location >> 3] &= ~(1 << (location & 7));
				}
				//输出经过染色后的矩阵D
				PrintReceiverU8Data(matrixDelta[i], "matrix D[i]", heightInBytes);
			}
			std::cout << "Mark the value of D[i][v_y[i]] in matrix D as 0\n";



			//////////////// Compute matrix A & sent matrix ///////////////////////

			u8* sentMatrix[w];

			for (auto i = 0; i < w; ++i) {
				PRNG prng(otMessages[i + wLeft][0]);
				prng.get(matrixA[i], heightInBytes);

				sentMatrix[i] = new u8[heightInBytes];
				prng.SetSeed(otMessages[i + wLeft][1]);
				prng.get(sentMatrix[i], heightInBytes);

				for (auto j = 0; j < heightInBytes; ++j) {
					sentMatrix[i][j] ^= matrixA[i][j] ^ matrixDelta[i][j];
				}

				// ch.asyncSend(sentMatrix[i], heightInBytes);
				// write(reciever2senderfd, sentMatrix[i], heightInBytes);

				int total;
				total = heightInBytes;
				int write_size;
				while (1){
					write_size = write(reciever2senderfd, sentMatrix[i] + heightInBytes - total, total);
					if(write_size < 0){
						std::cout << "error \n";
						break;
					}
					if(total == write_size){
						break;
					}
					total = total - write_size;
				}


				// write(reciever2senderfd, "\r$_$\n", strlen("\r$_$\n"));






				//输出待发送矩阵sentMatrix
				PrintReceiverU8Data(sentMatrix[i], "matrix (A, B)[i]", heightInBytes);
			}

			std::cout << "Calculate the matrix A and B, and use OT to send it to the data sender\n";



			///////////////// Compute hash inputs (transposed) /////////////////////

			for (auto i = 0; i < w; ++i) {
				for (auto j = 0; j < receiverSize; ++j) {
					auto location = (*(u32*)(transLocations[i] + j * locationInBytes)) & shift;

					transHashInputs[i + wLeft][j >> 3] |= (u8)((bool)(matrixA[i][location >> 3] & (1 << (location & 7)))) << (j & 7);
				}
				//输出A[i][v_y[i]]
				PrintReceiverU8Data(transHashInputs[i + wLeft], "A[i][v_y[i]]", receiverSizeInBytes);
			}
			std::cout << "Record the value of A[i][v_y[i]]\n";

		}
		close(reciever2senderfd);

		std::cout << "Receiver matrix sent and transposed hash input computed\n";
		timer.setTimePoint("Receiver matrix sent and transposed hash input computed");




		/////////////////// Compute hash outputs ///////////////////////////

		RandomOracle H(hashLengthInBytes);
		u8 hashOutput[sizeof(block)];
		std::unordered_map<u64, std::vector<std::pair<block, u32>>> allHashes;
		u8* hashInputs[bucket2];
		for (auto i = 0; i < bucket2; ++i) {
			hashInputs[i] = new u8[widthInBytes];
		}

		for (auto low = 0; low < receiverSize; low += bucket2) {
			auto up = low + bucket2 < receiverSize ? low + bucket2 : receiverSize;

			for (auto j = low; j < up; ++j) {
				memset(hashInputs[j - low], 0, widthInBytes);
			}

			for (auto i = 0; i < width; ++i) {
				for (auto j = low; j < up; ++j) {
					hashInputs[j - low][i >> 3] |= (u8)((bool)(transHashInputs[i][j >> 3] & (1 << (j & 7)))) << (i & 7);
				}
			}
			u8* recBuff = new u8[(up - low) * hashLengthInBytes];
			for (auto j = low; j < up; ++j) {
				H.Reset();
				H.Update(hashInputs[j - low], widthInBytes);
				H.Final(hashOutput);
				allHashes[*(u64*)hashOutput].push_back(std::make_pair(*(block*)hashOutput, j));

				memcpy(recBuff + (j - low) * hashLengthInBytes, hashOutput, hashLengthInBytes);
			}
			//输出Ay
			PrintReceiverU8Data(recBuff, "Ay", hashLengthInBytes);
			std::cout << "Calculate Ay using A[i][v_y[i]]\n";
		}
		// std::cout << "Receiver hash outputs computed\n";
		timer.setTimePoint("Receiver hash outputs computed");



		///////////////// Receive hash outputs from sender and compute PSI ///////////////////

		u8* recvBuff = new u8[bucket2 * hashLengthInBytes];
		auto psi = 0;
		std::vector<block> psiVector;

		for (auto low = 0; low < senderSize; low += bucket2) {
			auto up = low + bucket2 < senderSize ? low + bucket2 : senderSize;

			// ch.recv(recvBuff, (up - low) * hashLengthInBytes);
			// read(sender2recieverfd, recvBuff, (up - low) * hashLengthInBytes);
			int total;
			int t;
			t = (up - low) * hashLengthInBytes;
			total = t;
			int read_size;
			while (1){
				read_size = read(sender2recieverfd, recvBuff + t - total, total);
				if(total == read_size){
					break;
				}
				total = total - read_size;
			}



			//输出Cx
			PrintReceiverU8Data(recvBuff, "Cx", t);
			std::cout << "Receive the Cx sent by the data sender\n";

			for (auto idx = 0; idx < up - low; ++idx) {
				u64 mapIdx = *(u64*)(recvBuff + idx * hashLengthInBytes);

				auto found = allHashes.find(mapIdx);
				if (found == allHashes.end()) continue;
				for (auto i = 0; i < found->second.size(); ++i) {
					if (memcmp(&(found->second[i].first), recvBuff + idx * hashLengthInBytes, hashLengthInBytes) == 0) {
						psiVector.push_back(receiverSet[found->second[i].second]);
						++psi;
						break;
					}
				}

			}
		}
		timer.setTimePoint("Receiver intersection computed");
		std::cout << timer;


		//////////////// Output communication /////////////////

		u64 sentData = ch.getTotalDataSent();
		u64 recvData = ch.getTotalDataRecv();
		u64 totalData = sentData + recvData;

		std::cout << "Receiver sent communication: " << sentData / std::pow(2.0, 20) << " MB\n";
		std::cout << "Receiver received communication: " << recvData / std::pow(2.0, 20) << " MB\n";
		std::cout << "Receiver total communication: " << totalData / std::pow(2.0, 20) << " MB\n";
		std::cout << "psi count: " << psi << " \n";
		PSIRet ret;
		unsigned long long(* retArray)[2] = (unsigned long long (*)[2])malloc(sizeof(unsigned long long)*psi*2);
		for (auto i=0; i<psi; i++){
			retArray[i][1] = psiVector[i][1];
			retArray[i][0] = psiVector[i][0];
		}
		ret.length = psi;
		ret.PSISet = retArray;
		return ret;
	}




}

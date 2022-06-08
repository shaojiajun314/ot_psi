#include "PsiSenderForTest.h"

#include <iostream>
#include <cstdlib>
#include <bitset>
#include <ctime>

#include<stdio.h>
#include <unistd.h>
#include<fcntl.h>
#include<fstream>
#include<string>


namespace PSI {
	//输出u8数据
	void PrintSenderU8Data(u8* data, char* name, int length){
    block* tmp = (block*)(data);
		time_t now = time(nullptr);
		char* curr_time = ctime(&now);
		std::cout << curr_time;
		std::cout << "name:" << name << '\n';
    for (int i=0; i<length; i++){
			printf("%d",data[i]);
    }
		std::cout << "\n";
  }

	//输出block数据
	void PrintSenderBlockData(block* data, int number,char* name)
	{
		time_t now = time(nullptr);
		char* curr_time = ctime(&now);
		std::cout << curr_time;
		std::cout << "name:" << name << " number:" << number << '\n';
		for (int i = 0; i < number; ++i) {
			std::cout << data[i] << '\n';
		}
    }

	void PsiSenderForTest::run(PRNG& prng, Channel& ch, block commonSeed, const u64& senderSize, const u64& receiverSize, const u64& height, const u64& logHeight, const u64& width, std::vector<block>& senderSet, const u64& hashLengthInBytes, const u64& h1LengthInBytes, const u64& bucket1, const u64& bucket2) {

		int sender2receiverfd;
		int reciever2senderfd;
		// sender2receiverfd = open("t_sender2receiver", O_WRONLY);
		// reciever2senderfd = open("t_receiver2sender", O_RDONLY);
		sender2receiverfd = open("s_sender2receiver", O_WRONLY);
		reciever2senderfd = open("s_receiver2sender", O_RDONLY);



		Timer timer;
		timer.setTimePoint("Sender start");

		auto heightInBytes = (height + 7) / 8;
		auto widthInBytes = (width + 7) / 8;
		auto locationInBytes = (logHeight + 7) / 8;
		auto senderSizeInBytes = (senderSize + 7) / 8;
		auto shift = (1 << logHeight) - 1;
		auto widthBucket1 = sizeof(block) / locationInBytes;



		//////////////////// Base OTs /////////////////////////////////

		IknpOtExtReceiver otExtReceiver;
		otExtReceiver.genBaseOts(prng, ch);
		BitVector choices(width);
		std::vector<block> otMessages(width);
		prng.get(choices.data(), choices.sizeBytes());
		otExtReceiver.receive(choices, otMessages, prng, ch);

		// std::cout << "Sender base OT finished\n";
		timer.setTimePoint("Sender base OT finished");




		////////////// Initialization //////////////////////

		PRNG commonPrng(commonSeed);
		block commonKey;
		AES commonAes;

		u8* transLocations[widthBucket1];
		for (auto i = 0; i < widthBucket1; ++i) {
			transLocations[i] = new u8[senderSize * locationInBytes + sizeof(u32)];
		}

		block randomLocations[bucket1];

		u8* matrixC[widthBucket1];
		for (auto i = 0; i < widthBucket1; ++i) {
			matrixC[i] = new u8[heightInBytes];
		}

		u8* transHashInputs[width];
		for (auto i = 0; i < width; ++i) {
			transHashInputs[i] = new u8[senderSizeInBytes];
			memset(transHashInputs[i], 0, senderSizeInBytes);
		}




		/////////// Transform input /////////////////////

		commonPrng.get((u8*)&commonKey, sizeof(block));
		commonAes.setKey(commonKey);

		block* sendSet = new block[senderSize];
		block* aesInput = new block[senderSize];
		block* aesOutput = new block[senderSize];

		RandomOracle H1(h1LengthInBytes);
		u8 h1Output[h1LengthInBytes];

		for (auto i = 0; i < senderSize; ++i) {
			H1.Reset();
			H1.Update((u8*)(senderSet.data() + i), sizeof(block));
			H1.Final(h1Output);

			aesInput[i] = *(block*)h1Output;
			sendSet[i] = *(block*)(h1Output + sizeof(block));
		}

		commonAes.ecbEncBlocks(aesInput, senderSize, aesOutput);
		for (auto i = 0; i < senderSize; ++i) {
			sendSet[i] ^= aesOutput[i];
		}

		// std::cout << "Sender set transformed\n";
		//把元素集合中每一个元素x的v_x输出, 第i行就是第i个元素的v_x=F_k(H_1(x))
		PrintSenderBlockData(sendSet, senderSize, "v_x");
		std::cout << "For element x in data set X, the receiver uses AES to calculate the v_x=F_k(H_1(x)) function\n";
		timer.setTimePoint("Sender set transformed");





		for (auto wLeft = 0; wLeft < width; wLeft += widthBucket1) {
			auto wRight = wLeft + widthBucket1 < width ? wLeft + widthBucket1 : width;
			auto w = wRight - wLeft;

			//////////// Compute random locations (transposed) ////////////////

			commonPrng.get((u8*)&commonKey, sizeof(block));
			commonAes.setKey(commonKey);

			for (auto low = 0; low < senderSize; low += bucket1) {

				auto up = low + bucket1 < senderSize ? low + bucket1 : senderSize;

				commonAes.ecbEncBlocks(sendSet + low, up - low, randomLocations);

				for (auto i = 0; i < w; ++i) {
					for (auto j = low; j < up; ++j) {
						memcpy(transLocations[i] + j * locationInBytes, (u8*)(randomLocations + (j - low)) + i * locationInBytes, locationInBytes);
						//把元素集合中每一个元素x对于的染色位置输出
						PrintSenderU8Data((u8*)(randomLocations + (j - low)) + i * locationInBytes, "v_x[i]", locationInBytes);
					}
					//不同元素之间使用换行标识
					std::cout << "\n";
				}
			}
			std::cout << "Record the value of v_x[i]\n";




			//////////////// Extend OTs and compute matrix C ///////////////////

			u8* recvMatrix;
			recvMatrix = new u8[heightInBytes];
			// char tmp[heightInBytes] = {0};

			for (auto i = 0; i < w; ++i) {
				PRNG prng(otMessages[i + wLeft]);
				prng.get(matrixC[i], heightInBytes);

				// ch.recv(recvMatrix, heightInBytes);
				// read(reciever2senderfd, recvMatrix, heightInBytes);

				int total;
				total = heightInBytes;
				int read_size;
				while (1){
					read_size = read(reciever2senderfd, recvMatrix + heightInBytes - total, total);
					if(total == read_size){
						break;
					}
					total = total - read_size;
				}


				if (choices[i + wLeft]) {
					for (auto j = 0; j < heightInBytes; ++j) {
						matrixC[i][j] ^= recvMatrix[j];
					}
				}
				//输出矩阵C
				PrintSenderU8Data(matrixC[i], "matrix C[i]", heightInBytes);

			}
			std::cout << "Obtain matrix C from data receiver using OT\n";


			///////////////// Compute hash inputs (transposed) /////////////////////

			for (auto i = 0; i < w; ++i) {
				for (auto j = 0; j < senderSize; ++j) {
					auto location = (*(u32*)(transLocations[i] + j * locationInBytes)) & shift;

					transHashInputs[i + wLeft][j >> 3] |= (u8)((bool)(matrixC[i][location >> 3] & (1 << (location & 7)))) << (j & 7);
				}
				//输出C[i][v_x[i]]
				PrintSenderU8Data(transHashInputs[i + wLeft], "C[i][v_x[i]]", senderSizeInBytes);
			}
			std::cout << "Record the value of C[i][v_x[i]]\n";

		}

		// std::cout << "Sender transposed hash input computed\n";
		timer.setTimePoint("Sender transposed hash input computed");




		/////////////////// Compute hash outputs ///////////////////////////

		RandomOracle H(hashLengthInBytes);
		u8 hashOutput[sizeof(block)];

		u8* hashInputs[bucket2];

		for (auto i = 0; i < bucket2; ++i) {
			hashInputs[i] = new u8[widthInBytes];
		}

		for (auto low = 0; low < senderSize; low += bucket2) {
			auto up = low + bucket2 < senderSize ? low + bucket2 : senderSize;

			for (auto j = low; j < up; ++j) {
				memset(hashInputs[j - low], 0, widthInBytes);
			}

			for (auto i = 0; i < width; ++i) {
				for (auto j = low; j < up; ++j) {
					hashInputs[j - low][i >> 3] |= (u8)((bool)(transHashInputs[i][j >> 3] & (1 << (j & 7)))) << (i & 7);
				}
			}

			u8* sentBuff = new u8[(up - low) * hashLengthInBytes];

			for (auto j = low; j < up; ++j) {
				H.Reset();
				H.Update(hashInputs[j - low], widthInBytes);
				H.Final(hashOutput);

				memcpy(sentBuff + (j - low) * hashLengthInBytes, hashOutput, hashLengthInBytes);
			}

			// ch.asyncSend(sentBuff, (up - low) * hashLengthInBytes);
			// write(sender2receiverfd, sentBuff, (up - low) * hashLengthInBytes);


			int total;
			int t;
			t = (up - low) * hashLengthInBytes;
			total = t;
			int write_size;
			while (1){
				write_size = write(sender2receiverfd, sentBuff + t - total, total);
				if(write_size < 0){
					std::cout << "error \n";
					break;
				}
				if(total == write_size){
					break;
				}
				total = total - write_size;
			}
			// write(sender2receiverfd, "\r$_$\n", strlen("\r$_$\n"));

			//输出Cx
			PrintSenderU8Data(sentBuff, "Cx", t);
			std::cout << "Calculate Cx, and send Cx to the data receiver\n";
		}

		close(sender2receiverfd);

		// std::cout << "Sender hash outputs computed and sent\n";
		timer.setTimePoint("Sender hash outputs computed and sent");

		std::cout << timer;

	}

}

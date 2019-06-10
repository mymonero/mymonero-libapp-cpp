//
//  parsing.hpp
//  Copyright (c) 2014-2019, MyMonero.com
//
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without modification, are
//  permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice, this list of
//	conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice, this list
//	of conditions and the following disclaimer in the documentation and/or other
//	materials provided with the distribution.
//
//  3. Neither the name of the copyright holder nor the names of its contributors may be
//	used to endorse or promote products derived from this software without specific
//	prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
//  THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
//  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
//  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef parsing_hpp
#define parsing_hpp

#include <string>
#include <boost/optional/optional.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/foreach.hpp>
#include <iostream>
#include <ctime>
#include "rapidjson/stringbuffer.h"
#include <rapidjson/writer.h>
#include "./HTTPRequests_Interface.hpp"
#include "../Currencies/Currencies.hpp"
#include "../Wallets/Wallet_KeyImageCache.hpp"


namespace HostedMonero
{
	using namespace std;
	using namespace rapidjson;
	//
	// Constants - TODO? maybe move this out to proper domain
	static uint64_t txMinConfirms = 10; // Minimum number of confirmations for a transaction to show as confirmed
	static uint64_t maxBlockNumber = 500000000; // Maximum block number, used for tx unlock time
	//
	inline int parse_time_int(const char* value)
	{
		return std::strtol(value, nullptr, 10);
	}
	static inline std::time_t gm_time_from_ISO8601(const std::string& input)
	{
		constexpr const size_t expectedLength = sizeof("1234-12-12T12:12:12Z") - 1;
		static_assert(expectedLength == 20, "Unexpected ISO8601 date/time length");
		if (input.length() < expectedLength) {
			return 0;
		}
		std::tm time = { 0 };
		time.tm_year = parse_time_int(&input[0]) - 1900;
		time.tm_mon = parse_time_int(&input[5]) - 1;
		time.tm_mday = parse_time_int(&input[8]);
		time.tm_hour = parse_time_int(&input[11]);
		time.tm_min = parse_time_int(&input[14]);
		time.tm_sec = parse_time_int(&input[17]);
		time.tm_isdst = 0;
		const int millis = input.length() > 20 ? parse_time_int(&input[20]) : 0;
		//
		return timegm(&time)*1000 + millis;
	}
	//
	// SpentOutputDescription
	struct SpentOutputDescription; // forward decl
	ostream& operator<<(ostream& os, const SpentOutputDescription& obj); // declared pre-emptively for struct methods
	struct SpentOutputDescription
	{
		uint64_t amount;
		string tx_pub_key;
		string key_image;
		size_t mixin;
		uint64_t out_index;
		//
		bool operator==(const SpentOutputDescription &r) const
		{
			if (amount != r.amount) {
				return false;
			}
			if (tx_pub_key != r.tx_pub_key) {
				return false;
			}
			if (key_image != r.key_image) {
				return false;
			}
			if (mixin != r.mixin) {
				return false;
			}
			if (out_index != r.out_index) {
				return false;
			}
			return true;
		}
		bool operator!=(const SpentOutputDescription &r) const
		{
			return (*this == r) == false;
		}
		static SpentOutputDescription new_fromJSONRepresentation(const rapidjson::Value &dict)
		{
			assert(dict.IsObject());
			uint64_t amount = stoull(dict["amount"].GetString()); // stored as a string
			auto obj = SpentOutputDescription{
				amount,
				dict["tx_pub_key"].GetString(),
				dict["key_image"].GetString(),
				dict["mixin"].GetUint(),
				dict["out_index"].GetUint64()
			};
//			cout << "SpentOutputDescription new_fromJSONRepresentation -> " << obj << endl;
			//
			return obj;
		}
		static std::vector<SpentOutputDescription> newArray_fromJSONRepresentations(
			const rapidjson::Value &arrayValue
		) {
			std::vector<SpentOutputDescription> descs;
			for (auto &dict: arrayValue.GetArray())
			{
				assert(dict.IsObject());
				descs.push_back(SpentOutputDescription::new_fromJSONRepresentation(dict));
			}
			return descs;
		}
	};
	inline ostream& operator<<(ostream& os, const SpentOutputDescription& obj)
	{
		os << "SpentOutputDescription{" << endl;
		os
			<< "amount: " << obj.amount << endl
			<< "tx_pub_key: " << obj.tx_pub_key << endl
			<< "key_image: " << obj.key_image << endl
			<< "mixin: " << obj.mixin << endl
			<< "out_index: " << obj.out_index << endl
		;
		os << "}" << endl;
		//
		return os;
	}
	//
	// For API response parsing
//	static inline std::vector<spent_output_description> newArrayFrom_spentOutputDescriptions(
//		std::vector<ResponseJSON> api_json_dicts
//	) {
//		return dicts.map{ MoneroSpentOutputDescription.new(withAPIJSONDict: $0) }
//	}
//	static func new(withAPIJSONDict dict: [String: Any]) -> MoneroSpentOutputDescription
//	{
//		let instance = MoneroSpentOutputDescription(
//			amount: MoneroAmount(dict["amount"] as! String)!,
//			tx_pub_key: dict["tx_pub_key"] as! MoneroTransactionPubKey,
//			key_image: dict["key_image"] as! MoneroKeyImage,
//			mixin: dict["mixin"] as! UInt,
//			out_index: dict["out_index"] as! UInt64
//		)
//		return instance
//	}
	//
	// Persistence serialization
	static rapidjson::Value jsonRepresentation(const SpentOutputDescription &desc, Document::AllocatorType &allocator)
	{
		rapidjson::Value dict;
		dict.SetObject();
		{
			ostringstream oss;
			oss << desc.amount; // assume storing as a 'unsigned long long' in a string
			Value v(oss.str(), allocator);
			dict.AddMember("amount", v.Move(), allocator);
		}
		{
			Value v(desc.tx_pub_key, allocator);
			dict.AddMember("tx_pub_key", v.Move(), allocator);
		}
		{
			Value v(desc.key_image, allocator);
			dict.AddMember("key_image", v.Move(), allocator);
		}
		{
			Value v;
			v.SetUint(desc.mixin);
			dict.AddMember("mixin", v.Move(), allocator);
		}
		{
			Value v;
			v.SetUint64(desc.out_index);
			dict.AddMember("out_index", v.Move(), allocator);
		}
		//
		return dict;
	}
	static rapidjson::Value new_arrayOfSerializedDicts(
		const std::vector<SpentOutputDescription> &array,
		Document::AllocatorType &allocator
	) {
		Value serialized_dicts(kArrayType);
		BOOST_FOREACH(const SpentOutputDescription &desc, array)
		{
			serialized_dicts.PushBack(jsonRepresentation(desc, allocator).Move(), allocator);
		}
		return serialized_dicts;
	}
	//
	// HistoricalTxRecord
	class HistoricalTxRecord; // forward decl
	ostream& operator<<(ostream& os, const HistoricalTxRecord& obj); // declared pre-emptively for class methods
	class HistoricalTxRecord
	{
	public:
		//
		// Properties
		int64_t amount; // not unsigned since it can be negative (outgoing; received < sent)
		uint64_t totalSent;
		uint64_t totalReceived;
		double approxFloatAmount;
		optional<std::vector<SpentOutputDescription>> spent_outputs;
		time_t timestamp;
		string hash;
		optional<string> paymentId; // this is made mutable so it can be recovered if saved only locally
		optional<size_t> mixin; // this is made mutable so it can be recovered if saved only locally
		//
		bool mempool;
		double unlock_time;
		optional<uint64_t> height; // may not have made it into a block yet!
		//
		// Transient values
		bool cached__isConfirmed;
		bool cached__isUnlocked;
//		optional<string> cached__lockedReason; // only calculated if isUnlocked=true
		//
		bool isJustSentTransientTransactionRecord; // allowed to be mutable for modification during tx cleanup
		optional<string> tx_key;
		optional<uint64_t> tx_fee;
		optional<string> to_address;
		optional<bool> isFailed; // set to mutable to allow changing in-place
		//
		// Lifecycle - Deinit
//		deinit
//		{
//			//		DDLog.TearingDown("MyMoneroCore", "Tearing down a \(self).")
//			//
//			NotificationCenter.default.post(name: NotificationNames.willBeDeinitialized.notificationName, object: self)
//		}
		//
		// Static - Accessors - Transforms
		static bool isConfirmed(
			optional<uint64_t> height,
			uint64_t blockchain_height
		) {
			if (height == none) {
				return false; // hasn't made it into a block yet
			}
			if (*height > blockchain_height) { // we'd get a negative number
				// this is probably a tx which is still pending
				return false;
			}
			uint64_t differenceInHeight = blockchain_height - *height;
			//
			return differenceInHeight > txMinConfirms;
		}
		static bool isUnlocked(
			double unlock_time,
			uint64_t blockchain_height
		) {
			if (unlock_time < (double)maxBlockNumber) { // then unlock time is block height
				return (double)blockchain_height >= unlock_time;
			} else { // then unlock time is s timestamp as TimeInterval
				double s_since_epoch = (double)std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now()).time_since_epoch().count(); // cryptonote_utils.js wrapped this in a round()… Do we need it?
				//
				return s_since_epoch >= unlock_time;
			}
		}
		// NOTE: Probably keep this at application-level so that it can be localized
//		static string lockedReason(
//			 double unlock_time,
//			 uint64_t blockchain_height
//		 ) {
//			func colloquiallyFormattedDate(_ date: Date) -> String
//			{
//				let date_DateInRegion = DateInRegion(absoluteDate: date)
//				let date_fromNow_resultTuple = try! date_DateInRegion.colloquialSinceNow(style: .full) // is try! ok? (do we expect failures?)
//				let date_fromNow_String = date_fromNow_resultTuple.colloquial
//				//
//				return date_fromNow_String
//			}
//			if (unlock_time < Double(MoneroConstants.maxBlockNumber)) { // then unlock time is block height
//				let numBlocks = unlock_time - Double(blockchain_height)
//				if (numBlocks <= 0) {
//					return NSLocalizedString("Transaction is unlocked", comment: "")
//				}
//				let timeUntilUnlock_s = numBlocks * Double(MoneroConstants.avgBlockTime)
//				let unlockPrediction_Date = Date().addingTimeInterval(timeUntilUnlock_s)
//				let unlockPrediction_fromNow_String = colloquiallyFormattedDate(unlockPrediction_Date)
//				//
//				return String(format:
//							  NSLocalizedString("Will be unlocked in %d blocks, about %@", comment: "Will be unlocked in {number} blocks, about {duration of time plus 'from now'}"),
//							  numBlocks,
//							  unlockPrediction_fromNow_String
//							  )
//			}
//			let unlock_time_TimeInterval = TimeInterval(unlock_time)
//			// then unlock time is s timestamp as TimeInterval
//			let currentTime_s = round(Date().timeIntervalSince1970) // TODO: round was ported from cryptonote_utils.js; Do we need it?
//			let time_difference = unlock_time_TimeInterval - currentTime_s
//			if(time_difference <= 0) {
//				return NSLocalizedString("Transaction is unlocked", comment: "")
//			}
//			let unlockTime_Date = Date(timeIntervalSince1970: unlock_time_TimeInterval)
//			let unlockTime_fromNow_String = colloquiallyFormattedDate(unlockTime_Date)
//			//
//			return String(format: NSLocalizedString("Will be unlocked %@", comment: "Will be unlocked {duration of time plus 'from now'}"), unlockTime_fromNow_String)
//		}
		//
		bool operator==(const HistoricalTxRecord &r) const
		{
			if (amount != r.amount) {
				return false;
			}
			if (totalSent != r.totalSent) {
				return false;
			}
			if (totalReceived != r.totalReceived) {
				return false;
			}
			if (((spent_outputs == none || spent_outputs->size() == 0) && (r.spent_outputs != none && r.spent_outputs->size() != 0))
				|| (spent_outputs != none && spent_outputs->size() != 0 && (r.spent_outputs == none || r.spent_outputs->size() == 0))
				|| (spent_outputs != none && *spent_outputs != *r.spent_outputs)) {
				return false;
			}
			if (timestamp != r.timestamp) {
				return false;
			}
			if ((paymentId == none && r.paymentId != none)
				|| (paymentId != none && r.paymentId == none) ||
				(paymentId != none && *paymentId != *r.paymentId)) {
				return false;
			}
			if (hash != r.hash) {
				return false;
			}
			if ((mixin == none && r.mixin != none)
				|| (mixin != none && r.mixin == none) ||
				(mixin != none && *mixin != *r.mixin)) {
				return false;
			}
			if (mempool != r.mempool) {
				return false;
			}
			if (unlock_time != r.unlock_time) {
				return false;
			}
			if ((height == none && r.height != none)
				|| (height != none && r.height == none) ||
				(height != none && *height != *r.height)) {
				return false;
			}
			if (isFailed != r.isFailed) {
				return false;
			}
			return true;
		}
		bool operator!=(const HistoricalTxRecord &r) const
		{
			return (*this == r) == false;
		}
		static HistoricalTxRecord new_fromJSONRepresentation(
			const rapidjson::Value &dict,
			uint64_t wallet__blockchainHeight
		) {
			assert(dict.IsObject());
			//
			optional<uint64_t> height = none;
			{
				Value::ConstMemberIterator itr = dict.FindMember("height");
				if (itr != dict.MemberEnd()) {
					height = itr->value.GetUint64();
				}
			}
			double unlock_time = dict["unlock_time"].GetDouble();
			optional<bool> optl__isFailed = none;
			{
				Value::ConstMemberIterator itr = dict.FindMember("isFailed");
				if (itr != dict.MemberEnd()) {
					optl__isFailed = itr->value.GetBool();
				}
			}
			//
			bool isConfirmed = HistoricalTxRecord::isConfirmed(height, wallet__blockchainHeight);
			bool isUnlocked = HistoricalTxRecord::isUnlocked(unlock_time, wallet__blockchainHeight);
			//		let lockedReason: String? = !isUnlocked ? MoneroHistoricalTransactionRecord.lockedReason(
			//																								 givenTransactionUnlockTime: unlockTime,
			//																								 andWalletBlockchainHeight: wallet__blockchainHeight
			//																								 ) : nil
			optional<uint64_t> optl__tx_fee = none;
			{
				Value::ConstMemberIterator itr = dict.FindMember("tx_fee");
				if (itr != dict.MemberEnd()) {
					optl__tx_fee = stoull(itr->value.GetString());
				}
			}
			optional<string> optl__paymentId = none;
			{
				Value::ConstMemberIterator itr = dict.FindMember("paymentId");
				if (itr != dict.MemberEnd()) {
					optl__paymentId = itr->value.GetString();
				}
			}
			optional<string> optl__tx_key = none;
			{
				Value::ConstMemberIterator itr = dict.FindMember("tx_key");
				if (itr != dict.MemberEnd()) {
					optl__tx_key = itr->value.GetString();
				}
			}
			optional<string> optl__to_address = none;
			{
				Value::ConstMemberIterator itr = dict.FindMember("to_address");
				if (itr != dict.MemberEnd()) {
					optl__to_address = itr->value.GetString();
				}
			}
			
			auto obj = HistoricalTxRecord{
				stoll(dict["amount"].GetString()),
				stoull(dict["total_sent"].GetString()),
				stoull(dict["total_received"].GetString()),
				//
				dict["approx_float_amount"].GetDouble(),
				SpentOutputDescription::newArray_fromJSONRepresentations(dict["spent_outputs"]),
				stol(dict["timestamp"].GetString()) * 1000, // this is assuming it's been stored in seconds, not milliseconds
				dict["hash"].GetString(),
				optl__paymentId,
				dict["mixin"].GetUint(),
				dict["mempool"].GetBool(),
				unlock_time,
				height,
				//
				isConfirmed,
				isUnlocked,
				//			cached__lockedReason: lockedReason,
				//
				false, // isJustSentTransientTransactionRecord
				optl__tx_key,
				optl__tx_fee,
				optl__to_address,
				optl__isFailed
			};
//			cout << "HistoricalTxRecord new_fromJSONRepresentation -> " << obj << endl;
			//
			return obj;
		}
		static std::vector<HistoricalTxRecord> newArray_fromJSONRepresentations(
			const rapidjson::Value &arrayValue,
			uint64_t wallet__blockchainHeight
		) {
			std::vector<HistoricalTxRecord> descs;
			for (auto &dict: arrayValue.GetArray())
			{
				assert(dict.IsObject());
				descs.push_back(
					HistoricalTxRecord::new_fromJSONRepresentation(dict, wallet__blockchainHeight)
				);
			}
			return descs;
		}
	};

	//
	// Accessory functions
	static inline bool sorting_historicalTxRecords_byTimestamp_walletsList(
		const HistoricalTxRecord &a,
		const HistoricalTxRecord &b
	) {
		// there are no ids here for sorting so we'll use timestamp
		// and .mempool can mess with user's expectation of tx sorting
		// when .isFailed is involved, so just going with a simple sort here
		return b.timestamp < a.timestamp;
	}	//
	// Persistence serialization
	static rapidjson::Value jsonRepresentation(
		const HistoricalTxRecord &desc,
		Document::AllocatorType &allocator
	) {
		rapidjson::Value dict;
		dict.SetObject();
		{
			ostringstream oss;
			oss << desc.amount; // assume storing as a 'unsigned long long' in a string
			Value v(oss.str(), allocator);
			dict.AddMember("amount", v.Move(), allocator);
		}
		{
			ostringstream oss;
			oss << desc.totalSent; // assume storing as a 'unsigned long long' in a string
			Value v(oss.str(), allocator);
			dict.AddMember("total_sent", v.Move(), allocator);
		}
		{
			ostringstream oss;
			oss << desc.totalReceived; // assume storing as a 'unsigned long long' in a string
			Value v(oss.str(), allocator);
			dict.AddMember("total_received", v.Move(), allocator);
		}
		{
			Value v;
			v.SetDouble(desc.approxFloatAmount);
			dict.AddMember("approx_float_amount", v.Move(), allocator);
		}
		{
			Value k("spent_outputs", allocator);
			std::vector<SpentOutputDescription> empty__spent_outputs;
//			auto serializedDicts = new_arrayOfSerializedDicts(
//															  desc.spent_outputs != none ? *(desc.spent_outputs) : empty__spent_outputs,
//															  allocator
//															  );
//			if (serializedDicts.Size() > 0) {
//				cout << "serialized spent_output" << endl;
//				StringBuffer buffer;
//				Writer<StringBuffer> writer(buffer);
//				serializedDicts.Accept(writer);
//				cout << buffer.GetString() << endl;
//			}
			dict.AddMember(
				k,
				new_arrayOfSerializedDicts(
					desc.spent_outputs != none ? *(desc.spent_outputs) : empty__spent_outputs,
					allocator
				).Move(),
				allocator
			);
		}
		{
			Value k("timestamp", allocator);
			ostringstream oss;
			oss << (desc.timestamp / 1000); // assume storing as a 'long' in a string; and SECONDS, so convert from time_t's milliseconds to seconds
			Value v(oss.str(), allocator);
			dict.AddMember(k, v.Move(), allocator);
		}
		{
			Value v(desc.hash, allocator); // copy
			dict.AddMember("hash", v.Move(), allocator);
		}
		{
			Value v(desc.mempool);
			dict.AddMember("mempool", v.Move(), allocator);
		}
		{
			Value v;
			v.SetDouble(desc.unlock_time);
			dict.AddMember("unlock_time", v.Move(), allocator);
		}
		if (desc.mixin != none) {
			Value v;
			v.SetUint(*(desc.mixin));
			dict.AddMember("mixin", v.Move(), allocator);
		}
		if (desc.height != none) {
			Value v;
			v.SetUint64(*(desc.height));
			dict.AddMember("height", v.Move(), allocator);
		}
		if (desc.paymentId != none) {
			Value v(*(desc.paymentId), allocator); // copy
			dict.AddMember("paymentId", v.Move(), allocator);
		}
		if (desc.tx_key != none) {
			Value v(*(desc.tx_key), allocator); // copy
			dict.AddMember("tx_key", v.Move(), allocator);
		}
		if (desc.tx_fee != none) {
			ostringstream oss;
			oss << *(desc.tx_fee); // assume storing as a 'unsigned long long' in a string
			Value v(oss.str(), allocator);
			dict.AddMember("tx_fee", v.Move(), allocator);
		}
		if (desc.to_address != none) {
			Value v(*(desc.to_address), allocator); // copy
			dict.AddMember("to_address", v.Move(), allocator);
		}
		if (desc.isFailed != none) {
			Value v(*(desc.isFailed));
			dict.AddMember("isFailed", v.Move(), allocator);
		}
		//
//		cout << "Whole serialized tx... " << endl;
//		{
//			StringBuffer buffer;
//			Writer<StringBuffer> writer(buffer);
//			dict.Accept(writer);
//			cout << buffer.GetString() << endl;
//		}
		//
		return dict;
	}
	static rapidjson::Value new_arrayOfSerializedDicts(
		const std::vector<HistoricalTxRecord> &array,
		Document::AllocatorType &allocator
	) {
		Value serialized_dicts(kArrayType);
		BOOST_FOREACH(const HistoricalTxRecord &desc, array)
		{
			serialized_dicts.PushBack(jsonRepresentation(desc, allocator).Move(), allocator);
		}
		return serialized_dicts;
	}
	//
	//
	inline ostream& operator<<(ostream& os, const HistoricalTxRecord& obj)
	{
		os << "HistoricalTxRecord{" << endl;
		os
			<< "amount: " << obj.amount << endl
			<< "totalSent: " << obj.totalSent << endl
			<< "totalReceived: " << obj.totalReceived << endl
			<< "approxFloatAmount: " << obj.approxFloatAmount << endl;
		os
		<< "spent_outputs: " << (obj.spent_outputs == none ? "--" : obj.spent_outputs->size() == 0 ? "empty" : "") << endl;
		if (obj.spent_outputs != none) {
			size_t i = 0;
			BOOST_FOREACH(const SpentOutputDescription &desc, (*obj.spent_outputs))
			{
				os << "\t" << i << ": " << desc << endl;
				i += 1;
			}
		}
		os
			<< "timestamp: " << obj.timestamp << endl
			<< "hash: " << obj.hash << endl
			<< "paymentId: " << obj.paymentId << endl
			<< "mixin: " << obj.mixin << endl
			//
			<< "mempool: " << obj.mempool << endl
			<< "unlock_time: " << obj.unlock_time << endl
			<< "height: " << obj.height << endl
			//
			<< "cached__isConfirmed: " << obj.cached__isConfirmed << endl
			<< "cached__isUnlocked: " << obj.cached__isUnlocked << endl
			//
			<< "isJustSentTransientTransactionRecord: " << obj.isJustSentTransientTransactionRecord << endl
			<< "tx_key: " << obj.tx_key << endl
			<< "tx_fee: " << obj.tx_fee << endl
			<< "to_address: " << obj.to_address << endl
			<< "isFailed: " << obj.isFailed << endl
		;
		os << "}" << endl;
		//
		return os;
	}
}
namespace HostedMonero
{
	struct ParsedResult_Login
	{
		bool isANewAddressToServer;
		optional<bool> generated_locally; // may be nil if the server doesn't support it yet (pre summer 18)
		optional<uint64_t> start_height; // may be nil if the server doesn't support it yet (pre summer 18)
	};
	static inline ParsedResult_Login new_ParsedResult_Login(
		const HTTPRequests::ResponseJSON &res
	) {
		optional<bool> generated_locally = none;
		{
			Value::ConstMemberIterator itr = res.FindMember("generated_locally");
			if (itr != res.MemberEnd()) {
				generated_locally = itr->value.GetBool();
			}
		}
		optional<uint64_t> start_height = none;
		{
			Value::ConstMemberIterator itr = res.FindMember("start_height");
			if (itr != res.MemberEnd()) {
				start_height = itr->value.GetUint64();
			}
		}
		return ParsedResult_Login{
			res["new_address"].GetBool(), // isANewAddressToServer
			generated_locally,
			start_height
		};
	}
	//
	struct ParsedResult_AddressInfo
	{
		uint64_t totalReceived;
		uint64_t totalSent;
		uint64_t lockedBalance;
		//
		uint64_t account_scanned_tx_height;
		uint64_t account_scanned_block_height;
		uint64_t account_scan_start_height;
		uint64_t transaction_height;
		uint64_t blockchain_height;
		//
		std::vector<SpentOutputDescription> spentOutputs; // these have a different format than MoneroOutputDescriptions (whose type's name needs to be made more precise)
		//
		std::unordered_map<Currencies::Currency, double> xmrToCcyRatesByCcy;
	};
	static inline ParsedResult_AddressInfo new_ParsedResult_AddressInfo(
		const HTTPRequests::ResponseJSON &res,
		const string &address,
		const string &view_key__private,
		const string &spend_key__public,
		const string &spend_key__private,
		std::shared_ptr<Wallets::KeyImageCache> keyImageCache
	) {
		uint64_t total_received = stoull(res["total_received"].GetString());
		uint64_t locked_balance = stoull(res["locked_funds"].GetString());
		uint64_t total_sent = stoull(res["total_sent"].GetString()); // gets modified in-place
		//
		uint64_t account_scanned_tx_height = res["scanned_height"].GetUint64();
		uint64_t account_scanned_block_height = res["scanned_block_height"].GetUint64();
		uint64_t account_scan_start_height = res["start_height"].GetUint64();
		uint64_t transaction_height = res["transaction_height"].GetUint64();
		uint64_t blockchain_height = res["blockchain_height"].GetUint64();
		//
		std::vector<SpentOutputDescription> spentOutputs;
		Value::ConstMemberIterator spent_outputs__itr = res.FindMember("spent_outputs");
		if (spent_outputs__itr != res.MemberEnd()) {
			if (spent_outputs__itr->value.IsNull() != true) { // this is actually a thing with freshly created wallets
				for (auto &spent_output: spent_outputs__itr->value.GetArray()) {
					assert(spent_output.IsObject());
					auto generated__keyImage = keyImageCache->lazy_keyImage(
						spent_output["tx_pub_key"].GetString(),
						spent_output["out_index"].GetUint64(),
						address,
						view_key__private,
						spend_key__private,
						spend_key__public
					);
					string spent_output__keyImage = spent_output["key_image"].GetString();
					if (spent_output__keyImage != generated__keyImage) { // not spent
						MDEBUG("HostedMonero: Output used as mixin \(spent_output__keyImage)/\(generated__keyImage))");
						uint64_t spent_output__amount = stoull(spent_output["amount"].GetString());
						total_sent -= spent_output__amount;
					}
					// TODO: this is faithful to old web wallet code but is it really correct?
					spentOutputs.push_back( // but keep output regardless of whether spent or not
						SpentOutputDescription::new_fromJSONRepresentation(spent_output)
					);
				}
			}
		}
		//
		std::unordered_map<Currencies::Currency, double> final_xmrToCcyRatesByCcy;
		Value::ConstMemberIterator rates__itr = res.FindMember("rates");
		if (rates__itr != res.MemberEnd()) { // jic it's not there
			for (auto& m : rates__itr->value.GetObject()) {
				auto ccySymbol = m.name.GetString();
				double xmrToCcyRate = m.value.GetDouble();
				Currencies::Currency ccy = Currencies::Currency::none; // initialized to zero value
				try {
					ccy = Currencies::CurrencyFrom(ccySymbol); // may throw - and will throw on 'BTC' in response
				} catch (const std::exception& e) {
					MWARNING("HostedMonero.APIClient: Unrecognized currency " << ccySymbol << " in rates matrix");
					continue; // already logged
				}
				if (ccy == Currencies::Currency::none) { // we shouldn't actually see this - we're expecting it to throw .. so let's throw here
					BOOST_THROW_EXCEPTION(logic_error("HostedMonero.APIClient: Unexpectedly unrecognized currency in rates matrix"));
					continue;
				}
				final_xmrToCcyRatesByCcy[ccy] = xmrToCcyRate;
			}
		}
		return ParsedResult_AddressInfo{
			total_received,
			total_sent,
			locked_balance,
			//
			account_scanned_tx_height,
			account_scanned_block_height,
			account_scan_start_height,
			transaction_height,
			blockchain_height,
			//
			spentOutputs,
			//
			final_xmrToCcyRatesByCcy
		};
	}
	//
	struct ParsedResult_AddressTransactions
	{
		uint64_t account_scanned_height;
		uint64_t account_scanned_block_height;
		uint64_t account_scan_start_height;
		uint64_t transaction_height;
		uint64_t blockchain_height;
		//
		std::vector<HistoricalTxRecord> transactions;
	};
	
	static inline ParsedResult_AddressTransactions new_ParsedResult_AddressTransactions(
		const HTTPRequests::ResponseJSON &res,
		const string &address,
		const string &view_key__private,
		const string &spend_key__public,
		const string &spend_key__private,
		std::shared_ptr<Wallets::KeyImageCache> keyImageCache
	) {
		uint64_t account_scanned_tx_height = res["scanned_height"].GetUint64();
		uint64_t account_scanned_block_height = res["scanned_block_height"].GetUint64();
		uint64_t account_scan_start_height = res["start_height"].GetUint64();
		uint64_t transaction_height = res["transaction_height"].GetUint64();
		uint64_t blockchain_height = res["blockchain_height"].GetUint64();
		//
		std::vector<HistoricalTxRecord> mutable_transactions;
		Value::ConstMemberIterator transactions__itr = res.FindMember("transactions");
		if (transactions__itr != res.MemberEnd()) {
			for (auto &tx_dict: transactions__itr->value.GetArray()) {
				assert(tx_dict.IsObject());
				assert(blockchain_height != 0);  // if we have txs to parse, I think we can assume height != 0
				//
				int64_t mutable__tx_total_sent = stoll(tx_dict["total_sent"].GetString()); // Critical: this must be able to go negative!!
				std::vector<SpentOutputDescription> spentOutputs;
				Value::ConstMemberIterator spent_outputs__itr = tx_dict.FindMember("spent_outputs");
				if (spent_outputs__itr != tx_dict.MemberEnd()) {
					for (auto &spent_output: spent_outputs__itr->value.GetArray()) {
						assert(spent_output.IsObject());
						auto generated__keyImage = keyImageCache->lazy_keyImage(
							spent_output["tx_pub_key"].GetString(),
							spent_output["out_index"].GetUint64(),
							address,
							view_key__private,
							spend_key__private,
							spend_key__public
						);
						string spent_output__keyImage = spent_output["key_image"].GetString();
						if (spent_output__keyImage != generated__keyImage) { // is NOT own - discard/redact
//							cout << "Output used as mixin " << spent_output__keyImage << "/" << generated__keyImage << endl;
							uint64_t spent_output__amount = stoull(spent_output["amount"].GetString());
							mutable__tx_total_sent -= spent_output__amount;
						} else { // IS own - include/keep
							spentOutputs.push_back(SpentOutputDescription::new_fromJSONRepresentation(spent_output));
						}
					}
				}
				int64_t final_tx_totalSent = mutable__tx_total_sent;
				uint64_t final_tx_totalReceived = stoull(tx_dict["total_received"].GetString()); // assuming value exists - default to 0 if n
//				cout << "final_tx_totalReceived " << final_tx_totalReceived << endl;
//				cout << "final_tx_totalSent " << final_tx_totalSent << endl;
//				if (string(tx_dict["timestamp"].GetString()) == "") {
//					cout << "BREAK" << endl;
//				}
				if (final_tx_totalReceived + final_tx_totalSent <= 0) {
					continue; // skip
				}
				int64_t final_tx_amount = final_tx_totalReceived - final_tx_totalSent; // must be allowed to go negative
				optional<uint64_t> height = none;
				{
					Value::ConstMemberIterator itr = tx_dict.FindMember("height");
					if (itr != tx_dict.MemberEnd()) {
						height = itr->value.GetUint64();
					}
				}
				double unlockTime = 0;
				{
					Value::ConstMemberIterator itr = tx_dict.FindMember("unlock_time");
					if (itr != tx_dict.MemberEnd()) {
						unlockTime = itr->value.GetDouble();
					}
				}
				//
				bool isConfirmed = HistoricalTxRecord::isConfirmed(height, blockchain_height);
				bool isUnlocked = HistoricalTxRecord::isUnlocked(unlockTime, blockchain_height);
//				optional<string> lockedReason = !isUnlocked ? HistoricalTxRecord::lockedReason(
//					givenTransactionUnlockTime: unlockTime,
//					andWalletBlockchainHeight: blockchain_height
//				) : nil
				//
				double approxFloatAmount = Currencies::doubleFrom(final_tx_amount);
				//
				optional<string> final__paymentId = none;
				{
					Value::ConstMemberIterator itr = tx_dict.FindMember("payment_id");
					if (itr != tx_dict.MemberEnd()) {
						final__paymentId = itr->value.GetString();
					}
				}
				// ^-- still not final...:
				if (final__paymentId != none && final__paymentId->size() == 16) {
					// short (encrypted) pid
					if (approxFloatAmount < 0) { // outgoing
						final__paymentId = none; // need to filter these out .. because (afaik) after adding short pid scanning support, the server can't presently filter out short (encrypted) pids on outgoing txs ... not sure this is the optimal or 100% correct solution
					}
				}
				optional<size_t> mixin = none;
				{
					Value::ConstMemberIterator itr = tx_dict.FindMember("mixin");
					if (itr != tx_dict.MemberEnd()) {
						mixin = itr->value.GetUint();
					}
				}
				time_t gm_timestamp_time_t = gm_time_from_ISO8601(string(tx_dict["timestamp"].GetString()));
				//
				assert(final_tx_totalSent >= 0); // since we filtered, just before
				uint64_t assumed_positive_final_tx_totalSent = (uint64_t)final_tx_totalSent;
//				cout << tx_dict["timestamp"].GetString() << ": " << gm_timestamp_time_t << endl;
				mutable_transactions.push_back(HistoricalTxRecord{
					final_tx_amount,
					assumed_positive_final_tx_totalSent, // must use this as it's been adjusted for non-own outputs
					stoull(tx_dict["total_received"].GetString()),
					approxFloatAmount, // -> String -> Double
					spentOutputs, // this has been adjusted for non-own outputs
					gm_timestamp_time_t,
					tx_dict["hash"].GetString(),
					final__paymentId,
					mixin,
					//
					tx_dict["mempool"].GetBool(),
					unlockTime,
					height,
					//
					isConfirmed,
					isUnlocked,
					//					lockedReason,
					//
					false, // isJustSentTransientTransactionRecord
					//
					// local-only or sync-only metadata
					none,
					none,
					none,
					none // since we don't get this from the server
				});
			}
		}
		sort(mutable_transactions.begin(), mutable_transactions.end(), sorting_historicalTxRecords_byTimestamp_walletsList);
		//
		return ParsedResult_AddressTransactions{
			account_scanned_tx_height, // account_scanned_tx_height =? account_scanned_height
			account_scanned_block_height,
			account_scan_start_height,
			transaction_height,
			blockchain_height,
			//
			mutable_transactions
		};
	}
	//
	struct ParsedResult_ImportRequestInfoAndStatus
	{
		string payment_id;
		string payment_address;
		uint64_t import_fee;
		optional<string> feeReceiptStatus;
	};
}
#endif /* parsing_hpp */

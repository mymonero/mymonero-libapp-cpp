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
#include <boost/foreach.hpp>
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
	//
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
			return SpentOutputDescription{
				amount,
				dict["tx_pub_key"].GetString(),
				dict["key_image"].GetString(),
				dict["mixin"].GetUint(),
				dict["out_index"].GetUint64()
			};
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
	class HistoricalTxRecord
	{
	public:
		//
		// Properties
		uint64_t amount;
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
				|| *spent_outputs != *r.spent_outputs) {
				return false;
			}
			if (timestamp != r.timestamp) {
				return false;
			}
			if ((paymentId == none && r.paymentId != none)
				|| (paymentId != none && r.paymentId == none) ||
				*paymentId != *r.paymentId) {
				return false;
			}
			if (hash != r.hash) {
				return false;
			}
			if ((mixin == none && r.mixin != none)
				|| (mixin != none && r.mixin == none) ||
				*mixin != *r.mixin) {
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
				*height != *r.height) {
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
			//
			return HistoricalTxRecord{
				stoull(dict["amount"].GetString()),
				stoull(dict["total_sent"].GetString()),
				stoull(dict["total_received"].GetString()),
				//
				dict["approx_float_amount"].GetDouble(),
				SpentOutputDescription::newArray_fromJSONRepresentations(dict["spent_outputs"]),
				stol(dict["timestamp"].GetString()),
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
			oss << desc.timestamp; // assume storing as a 'long' in a string
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
		{
			Value v(*(desc.paymentId), allocator); // copy
			dict.AddMember("paymentId", v.Move(), allocator);
		}
		{
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
		
		
cout << "TODO" << endl;

		
		if (spent_outputs__itr != res.MemberEnd()) {
			for (auto &spent_output: spent_outputs__itr->value.GetArray())
			{
				assert(spent_output.IsObject());
				auto generated__keyImage = keyImageCache->lazy_keyImage(
					spent_output["tx_pub_key"].GetString(),
					spent_output["out_index"].GetUint64(),
					address,
					view_key__private,
					spend_key__private,
					spend_key__public
				);
//				let spent_output__keyImage = spent_output["key_image"] as! MoneroKeyImage
//				if spent_output__keyImage != generated__keyImage { // not spent
//					//				 DDLog.Info(
//					//					"HostedMonero",
//					//					"Output used as mixin \(spent_output__keyImage)/\(generated__keyImage))"
//					//				)
//					let spent_output__amount = MoneroAmount(spent_output["amount"] as! String)!
//					total_sent -= spent_output__amount
//				}
//				// TODO: this is faithful to old web wallet code but is it really correct?
//				spentOutputs.push_back( // but keep output regardless of whether spent or not
//											MoneroSpentOutputDescription.new(withAPIJSONDict: spent_output)
//											)
			}
		}
		//
//		let xmrToCcyRatesByCcySymbol = response_jsonDict["rates"] as? [String: Double] ?? [String: Double]() // jic it's not there
//		var final_xmrToCcyRatesByCcy: [CcyConversionRates.Currency: Double] = [:]
//		for (_, keyAndValueTuple) in xmrToCcyRatesByCcySymbol.enumerated() {
//			let ccySymbol = keyAndValueTuple.key
//			let xmrToCcyRate = keyAndValueTuple.value
//			guard let ccy = CcyConversionRates.Currency(rawValue: ccySymbol) else {
//				DDLog.Warn("HostedMonero.APIClient", "Unrecognized currency \(ccySymbol) in rates matrix")
//				continue
//			}
//			final_xmrToCcyRatesByCcy[ccy] = xmrToCcyRate
//		}
//		//
//		let result = ParsedResult_AddressInfo(
//											  totalReceived: total_received,
//											  totalSent: total_sent,
//											  lockedBalance: locked_balance,
//											  //
//											  account_scanned_tx_height: account_scanned_tx_height,
//											  account_scanned_block_height: account_scanned_block_height,
//											  account_scan_start_height: account_scan_start_height,
//											  transaction_height: transaction_height,
//											  blockchain_height: blockchain_height,
//											  //
//											  spentOutputs: spentOutputs,
//											  //
//											  xmrToCcyRatesByCcy: final_xmrToCcyRatesByCcy
//											  )
//		return (nil, result)








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
		cout << "TODO" << endl;

//		let account_scanned_tx_height = response_jsonDict["scanned_height"] as! UInt64
//		let account_scanned_block_height = response_jsonDict["scanned_block_height"] as! UInt64
//		let account_scan_start_height = response_jsonDict["start_height"] as! UInt64
//		let transaction_height = response_jsonDict["transaction_height"] as! UInt64
//		let blockchain_height = response_jsonDict["blockchain_height"] as! UInt64
//		//
//		var mutable_transactions: [MoneroHistoricalTransactionRecord] = []
//		let transaction_dicts = response_jsonDict["transactions"] as? [[String: Any]] ?? []
//		for (_, tx_dict) in transaction_dicts.enumerated() {
//			assert(blockchain_height != 0)  // if we have txs to parse, I think we can assume height != 0
//			//
//			var mutable__tx_total_sent = MoneroAmount(tx_dict["total_sent"] as! String)!
//			let spent_outputs: [[String: Any]] = tx_dict["spent_outputs"] as? [[String: Any]] ?? []
//			var mutable__final_tx_spent_output_dicts = [[String: Any]]()
//			for (_, spent_output) in spent_outputs.enumerated() {
//				let generated__keyImage = wallet_keyImageCache.lazy_keyImage(
//																			 tx_pub_key: spent_output["tx_pub_key"] as! MoneroTransactionPubKey,
//																			 out_index: spent_output["out_index"] as! UInt64,
//																			 public_address: address,
//																			 sec_keys: MoneroKeyDuo(view: view_key__private, spend: spend_key__private),
//																			 pub_spendKey: spend_key__public
//																			 )
//				let spent_output__keyImage = spent_output["key_image"] as! MoneroKeyImage
//				if spent_output__keyImage != generated__keyImage { // is NOT own - discard/redact
//					//					NSLog("Output used as mixin \(spent_output__keyImage)/\(generated__keyImage))")
//					let spent_output__amount = MoneroAmount(spent_output["amount"] as! String)!
//					mutable__tx_total_sent -= spent_output__amount
//				} else { // IS own - include/keep
//					mutable__final_tx_spent_output_dicts.append(spent_output)
//				}
//			}
//			let final_tx_totalSent: MoneroAmount = mutable__tx_total_sent
//			let final_tx_spent_output_dicts = mutable__final_tx_spent_output_dicts
//			let final_tx_totalReceived = MoneroAmount(tx_dict["total_received"] as! String)! // assuming value exists - default to 0 if n
//			if (final_tx_totalReceived + final_tx_totalSent <= 0) {
//				continue // skip
//			}
//			let final_tx_amount = final_tx_totalReceived - final_tx_totalSent
//
//			let height = tx_dict["height"] as? UInt64
//			let unlockTime = tx_dict["unlock_time"] as? Double ?? 0
//			//
//			let isConfirmed = MoneroHistoricalTransactionRecord.isConfirmed(
//																			givenTransactionHeight: height,
//																			andWalletBlockchainHeight: blockchain_height
//																			)
//			let isUnlocked = MoneroHistoricalTransactionRecord.isUnlocked(
//																		  givenTransactionUnlockTime: unlockTime,
//																		  andWalletBlockchainHeight: blockchain_height
//																		  )
//			let lockedReason: String? = !isUnlocked ? MoneroHistoricalTransactionRecord.lockedReason(
//																									 givenTransactionUnlockTime: unlockTime,
//																									 andWalletBlockchainHeight: blockchain_height
//																									 ) : nil
//			//
//			let approxFloatAmount = DoubleFromMoneroAmount(moneroAmount: final_tx_amount)
//			//
//			let record__payment_id = tx_dict["payment_id"] as? MoneroPaymentID
//			var final__paymentId = record__payment_id
//			if record__payment_id?.count == 16 {
//				// short (encrypted) pid
//				if approxFloatAmount < 0 {
//					// outgoing
//					final__paymentId = nil // need to filter these out .. because (afaik) after adding short pid scanning support, the server can't presently filter out short (encrypted) pids on outgoing txs ... not sure this is the optimal or 100% correct solution
//				}
//			}
//			let transactionRecord = MoneroHistoricalTransactionRecord(
//																	  amount: final_tx_amount,
//																	  totalSent: final_tx_totalSent, // must use this as it's been adjusted for non-own outputs
//																	  totalReceived: MoneroAmount("\(tx_dict["total_received"] as! String)")!,
//																	  approxFloatAmount: approxFloatAmount, // -> String -> Double
//																	  spent_outputs: MoneroSpentOutputDescription.newArray(
//																														   withAPIJSONDicts: final_tx_spent_output_dicts // must use this as it's been adjusted for non-own outputs
//																														   ),
//																	  timestamp: MoneroJSON_dateFormatter.date(from: "\(tx_dict["timestamp"] as! String)")!,
//																	  hash: tx_dict["hash"] as! MoneroTransactionHash,
//																	  paymentId: final__paymentId,
//																	  mixin: tx_dict["mixin"] as? UInt,
//																	  //
//																	  mempool: tx_dict["mempool"] as! Bool,
//																	  unlock_time: unlockTime,
//																	  height: height,
//																	  //
//																	  isFailed: nil, // since we don't get this from the server
//																	  //
//																	  cached__isConfirmed: isConfirmed,
//																	  cached__isUnlocked: isUnlocked,
//																	  cached__lockedReason: lockedReason,
//																	  //
//																	  //					id: (dict["id"] as? UInt64)!, // unwrapping this for clarity
//																	  isJustSentTransientTransactionRecord: false,
//																	  //
//																	  // local-only or sync-only metadata
//																	  tx_key: nil,
//																	  tx_fee: nil,
//																	  to_address: nil
//																	  )
//			mutable_transactions.append(transactionRecord)
//		}
//		mutable_transactions.sort { (a, b) -> Bool in
//			return a.timestamp >= b.timestamp // TODO: this used to sort by b.id-a.id in JS… is timestamp sort ok?
//		}
//		let final_transactions = mutable_transactions
//		//
//		let result = ParsedResult_AddressTransactions(
//													  account_scanned_height: account_scanned_tx_height, // account_scanned_tx_height =? account_scanned_height
//													  account_scanned_block_height: account_scanned_block_height,
//													  account_scan_start_height: account_scan_start_height,
//													  transaction_height: transaction_height,
//													  blockchain_height: blockchain_height,
//													  //
//													  transactions: final_transactions
//													  )
//		return (nil, result)
	}






	struct ParsedResult_ImportRequestInfoAndStatus
	{
		string payment_id;
		string payment_address;
		uint64_t import_fee;
		optional<string> feeReceiptStatus;
	};








}
#endif /* parsing_hpp */

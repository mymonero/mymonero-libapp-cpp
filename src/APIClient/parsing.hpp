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
#include "./HTTPRequests_Interface.hpp"
#include "../Currencies/Currencies.hpp"

namespace HostedMonero
{
	using namespace std;
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
	};
//	//
//	// For API response parsing
//	static inline std::vector<spent_output_description> newArrayFrom_spendOutputDescriptions(
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
//	//
//	// In-Swift serialization
//	static func newSerializedDictRepresentation(withArray array: [MoneroSpentOutputDescription]) -> [[String: Any]]
//	{
//		return array.map{ $0.jsonRepresentation }
//	}
//	var jsonRepresentation: [String: Any]
//	{
//		return [
//			"amount": String(amount, radix: 10),
//			"tx_pub_key": tx_pub_key,
//			"key_image": key_image,
//			"mixin": mixin,
//			"out_index": out_index
//		]
//	}
//	static func new(fromJSONRepresentation jsonRepresentation: [String: Any]) -> MoneroSpentOutputDescription
//	{
//		return self.init(
//			amount: MoneroAmount(jsonRepresentation["amount"] as! String)!,
//			tx_pub_key: jsonRepresentation["tx_pub_key"] as! MoneroTransactionPubKey,
//			key_image: jsonRepresentation["key_image"] as! MoneroKeyImage,
//			mixin: jsonRepresentation["mixin"] as! UInt,
//			out_index: jsonRepresentation["out_index"] as! UInt64
//		)
//	}
//	static func newArray(
//		fromJSONRepresentations array: [[String: Any]]
//	) -> [MoneroSpentOutputDescription] {
//		return array.map{ MoneroSpentOutputDescription.new(fromJSONRepresentation: $0) }
//	}

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
		// these properties are made mutable so they can be updated conveniently during a merge/sync from remote
		optional<string> tx_key;
		optional<uint64_t> tx_fee;
		optional<string> to_address;
		optional<bool> isFailed; // set to mutable to allow changing in-place
		//
		// Transient values
		bool cached__isConfirmed;
		bool cached__isUnlocked;
		optional<string> cached__lockedReason; // only calculated if isUnlocked=true
		bool isJustSentTransientTransactionRecord; // allowed to be mutable for modification during tx cleanup
		//
		// Lifecycle - Init
//		required init(
//					  amount: MoneroAmount,
//					  totalSent: MoneroAmount,
//					  totalReceived: MoneroAmount,
//					  approxFloatAmount: Double,
//					  spent_outputs: [MoneroSpentOutputDescription]?,
//					  timestamp: Date,
//					  hash: MoneroTransactionHash,
//					  paymentId: MoneroPaymentID?,
//					  mixin: UInt?,
//					  //
//					  mempool: Bool,
//					  unlock_time: Double,
//					  height: UInt64?,
//					  //
//					  isFailed: Bool?,
//					  cached__isConfirmed: Bool,
//					  cached__isUnlocked: Bool,
//					  cached__lockedReason: String?,
//					  isJustSentTransientTransactionRecord: Bool,
//					  //
//					  tx_key: MoneroTransactionSecKey?,
//					  tx_fee: MoneroAmount?,
//					  to_address: MoneroAddress?
//					  ) {
//			self.amount = amount
//			self.totalSent = totalSent
//			self.totalReceived = totalReceived
//			self.approxFloatAmount = approxFloatAmount
//			self.spent_outputs = spent_outputs
//			self.timestamp = timestamp
//			self.hash = hash
//			self.paymentId = paymentId
//			self.mixin = mixin
//			//
//			self.mempool = mempool
//			self.unlock_time = unlock_time
//			self.height = height
//			//
//			self.isFailed = isFailed
//			self.cached__isConfirmed = cached__isConfirmed
//			self.cached__isUnlocked = cached__isUnlocked
//			self.cached__lockedReason = cached__lockedReason
//			self.isJustSentTransientTransactionRecord = isJustSentTransientTransactionRecord
//			//
//			self.tx_key = tx_key
//			self.tx_fee = tx_fee
//			self.to_address = to_address
//		}
		//
		// Lifecycle - Deinit
//		deinit
//		{
//			//		DDLog.TearingDown("MyMoneroCore", "Tearing down a \(self).")
//			//
//			NotificationCenter.default.post(name: NotificationNames.willBeDeinitialized.notificationName, object: self)
//		}
//		//
//		// Static - Accessors - Transforms
//		static func isConfirmed(
//								givenTransactionHeight height: UInt64?,
//								andWalletBlockchainHeight blockchain_height: UInt64
//								) -> Bool {
//			if height == nil {
//				return false // hasn't made it into a block yet
//			}
//			if height! > blockchain_height { // we'd get a negative number
//				// this is probably a tx which is still pending
//				return false
//			}
//			let differenceInHeight = blockchain_height - height!
//			//
//			return differenceInHeight > MoneroConstants.txMinConfirms
//		}
//		static func isUnlocked(
//							   givenTransactionUnlockTime unlock_time: Double,
//							   andWalletBlockchainHeight blockchain_height: UInt64
//							   ) -> Bool {
//			if unlock_time < Double(MoneroConstants.maxBlockNumber) { // then unlock time is block height
//				return Double(blockchain_height) >= unlock_time
//			} else { // then unlock time is s timestamp as TimeInterval
//				let currentTime_s = round(Date().timeIntervalSince1970) // TODO: round was ported from cryptonote_utils.js; Do we need it?
//				return currentTime_s >= unlock_time
//			}
//				}
//				static func lockedReason(
//										 givenTransactionUnlockTime unlock_time: Double,
//										 andWalletBlockchainHeight blockchain_height: UInt64
//										 ) -> String {
//					func colloquiallyFormattedDate(_ date: Date) -> String
//					{
//						let date_DateInRegion = DateInRegion(absoluteDate: date)
//						let date_fromNow_resultTuple = try! date_DateInRegion.colloquialSinceNow(style: .full) // is try! ok? (do we expect failures?)
//						let date_fromNow_String = date_fromNow_resultTuple.colloquial
//						//
//						return date_fromNow_String
//					}
//					if (unlock_time < Double(MoneroConstants.maxBlockNumber)) { // then unlock time is block height
//						let numBlocks = unlock_time - Double(blockchain_height)
//						if (numBlocks <= 0) {
//							return NSLocalizedString("Transaction is unlocked", comment: "")
//						}
//						let timeUntilUnlock_s = numBlocks * Double(MoneroConstants.avgBlockTime)
//						let unlockPrediction_Date = Date().addingTimeInterval(timeUntilUnlock_s)
//						let unlockPrediction_fromNow_String = colloquiallyFormattedDate(unlockPrediction_Date)
//						//
//						return String(format:
//									  NSLocalizedString("Will be unlocked in %d blocks, about %@", comment: "Will be unlocked in {number} blocks, about {duration of time plus 'from now'}"),
//									  numBlocks,
//									  unlockPrediction_fromNow_String
//									  )
//					}
//					let unlock_time_TimeInterval = TimeInterval(unlock_time)
//					// then unlock time is s timestamp as TimeInterval
//					let currentTime_s = round(Date().timeIntervalSince1970) // TODO: round was ported from cryptonote_utils.js; Do we need it?
//					let time_difference = unlock_time_TimeInterval - currentTime_s
//					if(time_difference <= 0) {
//						return NSLocalizedString("Transaction is unlocked", comment: "")
//					}
//					let unlockTime_Date = Date(timeIntervalSince1970: unlock_time_TimeInterval)
//					let unlockTime_fromNow_String = colloquiallyFormattedDate(unlockTime_Date)
//					//
//					return String(format: NSLocalizedString("Will be unlocked %@", comment: "Will be unlocked {duration of time plus 'from now'}"), unlockTime_fromNow_String)
//				}
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
//				//
//				static func newSerializedDictRepresentation(
//															withArray array: [MoneroHistoricalTransactionRecord]
//															) -> [[String: Any]] {
//					return array.map{ $0.jsonRepresentation }
//				}
//				var jsonRepresentation: [String: Any]
//			{
//				var dict: [String: Any] =
//				[
//				 "amount": String(amount, radix: 10),
//				 "total_sent": String(totalSent, radix: 10),
//				 "total_received": String(totalReceived, radix: 10),
//				 //
//				 "approx_float_amount": approxFloatAmount,
//				 "spent_outputs": MoneroSpentOutputDescription.newSerializedDictRepresentation(
//																							   withArray: spent_outputs ?? []
//																							   ),
//				 "timestamp": timestamp.timeIntervalSince1970,
//				 "hash": hash,
//				 "mempool": mempool,
//				 "unlock_time": unlock_time
//				 ]
//				if mixin != nil {
//					dict["mixin"] = mixin
//				}
//				if height != nil {
//					dict["height"] = height
//				}
//				if let value = paymentId {
//					dict["paymentId"] = value
//				}
//				if let value = tx_key {
//					dict["tx_key"] = value
//				}
//				if let value = tx_fee {
//					dict["tx_fee"] = String(value, radix: 10)
//				}
//				if let value = to_address {
//					dict["to_address"] = value
//				}
//				if let value = isFailed {
//					dict["isFailed"] = value
//				}
//				//
//				return dict
//			}
//				static func new(
//								fromJSONRepresentation jsonRepresentation: [String: Any],
//								wallet__blockchainHeight: UInt64
//								) -> MoneroHistoricalTransactionRecord {
//					let height = jsonRepresentation["height"] as? UInt64
//					let unlockTime = jsonRepresentation["unlock_time"] as! Double
//					//
//					let isFailed = jsonRepresentation["isFailed"] as? Bool
//					let isConfirmed = MoneroHistoricalTransactionRecord.isConfirmed(
//																					givenTransactionHeight: height,
//																					andWalletBlockchainHeight: wallet__blockchainHeight
//																					)
//					let isUnlocked = MoneroHistoricalTransactionRecord.isUnlocked(
//																				  givenTransactionUnlockTime: unlockTime,
//																				  andWalletBlockchainHeight: wallet__blockchainHeight
//																				  )
//					let lockedReason: String? = !isUnlocked ? MoneroHistoricalTransactionRecord.lockedReason(
//																											 givenTransactionUnlockTime: unlockTime,
//																											 andWalletBlockchainHeight: wallet__blockchainHeight
//																											 ) : nil
//					var tx_fee: MoneroAmount? = nil
//					if let tx_fee_string = jsonRepresentation["tx_fee"] as? String {
//						tx_fee = MoneroAmount(tx_fee_string)!
//					}
//					//
//					return self.init(
//									 amount: MoneroAmount(jsonRepresentation["amount"] as! String)!,
//									 totalSent: MoneroAmount(jsonRepresentation["total_sent"] as! String)!,
//									 totalReceived: MoneroAmount(jsonRepresentation["total_received"] as! String)!,
//									 //
//									 approxFloatAmount: jsonRepresentation["approx_float_amount"] as! Double,
//									 spent_outputs: MoneroSpentOutputDescription.newArray(
//																						  fromJSONRepresentations: jsonRepresentation["spent_outputs"] as! [[String: Any]]
//																						  ),
//									 timestamp: Date(timeIntervalSince1970: jsonRepresentation["timestamp"] as! TimeInterval), // since we took .timeIntervalSince1970
//									 hash: jsonRepresentation["hash"] as! MoneroTransactionHash,
//									 paymentId: jsonRepresentation["paymentId"] as? MoneroPaymentID,
//									 mixin: jsonRepresentation["mixin"] as! UInt,
//									 mempool: jsonRepresentation["mempool"] as! Bool,
//									 unlock_time: unlockTime,
//									 height: height,
//									 //
//									 isFailed: isFailed,
//									 cached__isConfirmed: isConfirmed,
//									 cached__isUnlocked: isUnlocked,
//									 cached__lockedReason: lockedReason,
//									 //
//									 isJustSentTransientTransactionRecord: false,
//									 tx_key: jsonRepresentation["tx_key"] as? MoneroTransactionSecKey,
//									 tx_fee: tx_fee,
//									 to_address: jsonRepresentation["to_address"] as? String
//									 )
//				}
//				static func newArray(
//									 fromJSONRepresentations array: [[String: Any]],
//									 wallet__blockchainHeight: UInt64
//									 ) -> [MoneroHistoricalTransactionRecord] {
//					return array.map
//					{
//						return MoneroHistoricalTransactionRecord.new(
//																	 fromJSONRepresentation: $0,
//																	 wallet__blockchainHeight: wallet__blockchainHeight
//																	 )
//					}
//				}
	};
}
namespace HostedMonero
{
	struct ParsedResult_Login
	{
		bool isANewAddressToServer;
		optional<bool> generated_locally; // may be nil if the server doesn't support it yet (pre summer 18)
		optional<uint64_t> start_height; // may be nil if the server doesn't support it yet (pre summer 18)
	};
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
	struct ParsedResult_ImportRequestInfoAndStatus
	{
		string payment_id;
		string payment_address;
		uint64_t import_fee;
		optional<string> feeReceiptStatus;
	};

	
	//
//	static func newByParsing(
//							 response_jsonDict: [String: Any],
//							 address: MoneroAddress,
//							 view_key__private: MoneroKey,
//							 spend_key__public: MoneroKey,
//							 spend_key__private: MoneroKey,
//							 wallet_keyImageCache: MoneroUtils.KeyImageCache
//							 ) -> (
//								   err_str: String?,
//								   result: ParsedResult_AddressInfo?
//								   ) {
//		let total_received = MoneroAmount(response_jsonDict["total_received"] as! String)!
//		let locked_balance = MoneroAmount(response_jsonDict["locked_funds"] as! String)!
//		var total_sent = MoneroAmount(response_jsonDict["total_sent"] as! String)! // will get modified in-place
//		//
//		let account_scanned_tx_height = response_jsonDict["scanned_height"] as! UInt64
//		let account_scanned_block_height = response_jsonDict["scanned_block_height"] as! UInt64
//		let account_scan_start_height = response_jsonDict["start_height"] as! UInt64
//		let transaction_height = response_jsonDict["transaction_height"] as! UInt64
//		let blockchain_height = response_jsonDict["blockchain_height"] as! UInt64
//		let spent_outputs = response_jsonDict["spent_outputs"] as? [[String: Any]] ?? [[String: Any]]()
//		//
//		var mutable_spentOutputs: [MoneroSpentOutputDescription] = []
//		for (_, spent_output) in spent_outputs.enumerated() {
//			let generated__keyImage = wallet_keyImageCache.lazy_keyImage(
//																		 tx_pub_key: spent_output["tx_pub_key"] as! MoneroTransactionPubKey,
//																		 out_index: spent_output["out_index"] as! UInt64,
//																		 public_address: address,
//																		 sec_keys: MoneroKeyDuo(view: view_key__private, spend: spend_key__private),
//																		 pub_spendKey: spend_key__public
//																		 )
//			let spent_output__keyImage = spent_output["key_image"] as! MoneroKeyImage
//			if spent_output__keyImage != generated__keyImage { // not spent
//				//				 DDLog.Info(
//				//					"HostedMonero",
//				//					"Output used as mixin \(spent_output__keyImage)/\(generated__keyImage))"
//				//				)
//				let spent_output__amount = MoneroAmount(spent_output["amount"] as! String)!
//				total_sent -= spent_output__amount
//			}
//			// TODO: this is faithful to old web wallet code but is it really correct?
//			mutable_spentOutputs.append( // but keep output regardless of whether spent or not
//										MoneroSpentOutputDescription.new(withAPIJSONDict: spent_output)
//										)
//		}
//		let final_spentOutputs = mutable_spentOutputs
//		//
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
//											  spentOutputs: final_spentOutputs,
//											  //
//											  xmrToCcyRatesByCcy: final_xmrToCcyRatesByCcy
//											  )
//		return (nil, result)
//		}
	
//	static func newByParsing(
//							 response_jsonDict: [String: Any],
//							 address: MoneroAddress,
//							 view_key__private: MoneroKey,
//							 spend_key__public: MoneroKey,
//							 spend_key__private: MoneroKey,
//							 wallet_keyImageCache: MoneroUtils.KeyImageCache
//							 ) -> (
//								   err_str: String?,
//								   result: ParsedResult_AddressTransactions?
//								   ) {
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
//		}














}
#endif /* parsing_hpp */

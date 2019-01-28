//
//  SendFundsFormSubmissionController.cpp
//  MyMonero
//
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
//
#include "SendFundsFormSubmissionController.hpp"
#include <iostream>
#include "../../OpenAlias/OpenAlias.hpp"
#include "wallet_errors.h"
#include "monero_address_utils.hpp"
#include "monero_paymentID_utils.hpp"
#include "monero_send_routine.hpp"
using namespace monero_send_routine;
using namespace monero_transfer_utils;
using namespace SendFunds;

#include <boost/optional/optional_io.hpp>
//
// Imperatives - Initialization
void FormSubmissionController::set__get_unspent_outs_fn(std::function<void(LightwalletAPI_Req_GetUnspentOuts req_params)> fn)
{
	this->get_unspent_outs = fn;
}
void FormSubmissionController::set__get_random_outs_fn(std::function<void(LightwalletAPI_Req_GetRandomOuts req_params)> fn)
{
	this->get_random_outs = fn;
}
void FormSubmissionController::set__submit_raw_tx_fn(std::function<void(LightwalletAPI_Req_SubmitRawTx req_params)> fn)
{
	this->submit_raw_tx = fn;
}
void FormSubmissionController::set__authenticate_fn(std::function<void(void)> fn)
{
	this->authenticate_fn = fn;
}
//
// Imperatives - Controller
void FormSubmissionController::handle()
{
	using namespace std;
	using namespace boost;
	//
	THROW_WALLET_EXCEPTION_IF(this->valsState != WAIT_FOR_HANDLE, error::wallet_internal_error, "Expected valsState of WAIT_FOR_HANDLE");
	this->valsState = WAIT_FOR_STEP1;
	//
	if (this->parameters.fromWallet_didFailToInitialize) {
		this->parameters.failure_fn(unableToLoadWallet, boost::none, boost::none, boost::none, boost::none);
		return;
	}
	if (this->parameters.fromWallet_didFailToBoot) {
		this->parameters.failure_fn(unableToLogIntoWallet, boost::none, boost::none, boost::none, boost::none);
		return;
	}
	if (this->parameters.fromWallet_needsImport) {
		this->parameters.failure_fn(walletMustBeImported, boost::none, boost::none, boost::none, boost::none);
		return;
	}
	if (this->parameters.is_sweeping) {
		this->sending_amount = 0;
	} else {
		if (this->parameters.send_amount_double_string == boost::none || this->parameters.send_amount_double_string->empty()) {
			if (this->parameters.is_sweeping == false) {
				this->parameters.failure_fn(amountTooLow, boost::none, boost::none, boost::none, boost::none);
				return;
			}
		}
		bool ok = cryptonote::parse_amount(this->sending_amount, this->parameters.send_amount_double_string.get());
		if (!ok) {
			this->parameters.failure_fn(cannotParseAmount, boost::none, boost::none, boost::none, boost::none);
		}
		if (this->sending_amount <= 0) {
			this->parameters.failure_fn(amountTooLow, boost::none, boost::none, boost::none, boost::none);
			return;
		}
	}
	bool enteredAddressValue_exists = this->parameters.enteredAddressValue != boost::none && !this->parameters.enteredAddressValue->empty(); // it will be valid if it exists
//	let notPickedContactBut_enteredAddressValue = !hasPickedAContact && enteredAddressValue_exists ? true : false
	//
	bool resolvedAddress_exists = this->parameters.resolvedAddress != boost::none && !this->parameters.resolvedAddress->empty(); // NOTE: it might be hidden, though!
	//
	bool resolvedPaymentID_exists = this->parameters.resolvedPaymentID != boost::none && !this->parameters.resolvedPaymentID->empty(); // NOTE: it might be hidden, though!
	if (this->parameters.resolvedPaymentID_fieldIsVisible) {
		THROW_WALLET_EXCEPTION_IF(!resolvedPaymentID_exists, error::wallet_internal_error, "Expected resolvedPaymentID_exists && resolvedPaymentID_fieldIsVisible");
	}
	bool manuallyEnteredPaymentID_exists = this->parameters.manuallyEnteredPaymentID != boost::none && !this->parameters.manuallyEnteredPaymentID->empty();
	bool canUseManualPaymentID = manuallyEnteredPaymentID_exists
		&& this->parameters.manuallyEnteredPaymentID_fieldIsVisible
		&& !this->parameters.resolvedPaymentID_fieldIsVisible; // but not if we have a resolved one!
	if (canUseManualPaymentID && this->parameters.hasPickedAContact) {
		cerr << "canUseManualPaymentID shouldn't be true at same time as hasPickedAContact" << endl;
		// NOTE: That this is an exception will also be the case even if we are using the payment ID from a Funds Request QR code / URI because we set the request URI as a 'resolved' / "detected" payment id. So the `hasPickedAContact` usage above yields slight ambiguity in code and could be improved to encompass request uri pid "forcing"
		this->parameters.failure_fn(codeFault_manualPaymentID_while_hasPickedAContact, boost::none, boost::none, boost::none, boost::none);
		return;
	}
	optional<string> xmrAddress_toDecode = boost::none; // just initializing it to none here; may be integrated
	optional<string> paymentID_toUseOrToNilIfIntegrated = boost::none; // may be nil
	if (this->parameters.hasPickedAContact) { // we have already re-resolved the payment_id
		if (this->parameters.contact_payment_id != boost::none) {
			paymentID_toUseOrToNilIfIntegrated = this->parameters.contact_payment_id.get();
		}
		THROW_WALLET_EXCEPTION_IF(this->parameters.contact_hasOpenAliasAddress == boost::none, error::wallet_internal_error, "Expected non-none parameters.contact_hasOpenAliasAddress");
		if (*(this->parameters.contact_hasOpenAliasAddress) == true) {
			if (this->parameters.cached_OAResolved_address == boost::none || this->parameters.cached_OAResolved_address->empty()) { // ensure address indeed was able to be resolved - but UI should preclude it not having been by here
				cerr << "Code fault? Unable to find a recipient address for this transfer." << endl;
				this->parameters.failure_fn(codeFault_unableToFindResolvedAddrOnOAContact, boost::none, boost::none, boost::none, boost::none);
				return;
			}
			xmrAddress_toDecode = this->parameters.cached_OAResolved_address.get(); // We can just use the cached_OAResolved_XMR_address because in order to have picked this contact and for the user to hit send, we'd need to have gone through an OA resolve (_didPickContact)
		} else {
			xmrAddress_toDecode = this->parameters.contact_address.get();
		}
	} else {
		if (enteredAddressValue_exists == false) {
			this->parameters.failure_fn(pleaseSpecifyRecipient, boost::none, boost::none, boost::none, boost::none);
			return;
		}
		// address input via text input…
		bool is_enteredAddressValue_OAAddress = OpenAlias::looksLikeOpenAliasAndNotCryptoCurrencyAddress(this->parameters.enteredAddressValue.get());
		if (is_enteredAddressValue_OAAddress) {
			if (this->parameters.resolvedAddress_fieldIsVisible == false || resolvedAddress_exists == false) {
				this->parameters.failure_fn(couldntResolveThisOAAddress, boost::none, boost::none, boost::none, boost::none);
				return;
			}
			xmrAddress_toDecode = this->parameters.resolvedAddress.get();
			if (this->parameters.resolvedPaymentID_fieldIsVisible) {
				paymentID_toUseOrToNilIfIntegrated = this->parameters.resolvedPaymentID.get();
			} else if (canUseManualPaymentID) {
				paymentID_toUseOrToNilIfIntegrated = this->parameters.manuallyEnteredPaymentID.get();
			} else {
				paymentID_toUseOrToNilIfIntegrated = boost::none;
			}
		} else { // then it's an XMR address
			xmrAddress_toDecode = this->parameters.enteredAddressValue.get();
			// we don't care whether it's an integrated address or not here since we're not going to use its payment id
			if (canUseManualPaymentID) {
				if (this->parameters.resolvedPaymentID_fieldIsVisible) {
					cerr << "Code fault? Detected payment ID unexpectedly visible while manual input field visible." << endl;
					this->parameters.failure_fn(codeFault_detectedPIDVisibleWhileManualInputVisible, boost::none, boost::none, boost::none, boost::none);
					return;
				}
				paymentID_toUseOrToNilIfIntegrated = this->parameters.manuallyEnteredPaymentID.get();
			} else if (this->parameters.resolvedPaymentID_fieldIsVisible) {
				paymentID_toUseOrToNilIfIntegrated = this->parameters.resolvedPaymentID.get();
			}
		}
	}
	THROW_WALLET_EXCEPTION_IF(xmrAddress_toDecode == boost::none, error::wallet_internal_error, "Expected xmrAddress_toDecode");
	//
	auto decode_retVals = monero::address_utils::decodedAddress(xmrAddress_toDecode.get(), this->parameters.nettype);
	if (decode_retVals.did_error) {
		this->parameters.failure_fn(couldntValidateDestAddress, boost::none, boost::none, boost::none, boost::none);
		return;
	}
	if (decode_retVals.paymentID_string != boost::none) { // is integrated address!
		this->to_address_string = *xmrAddress_toDecode; // for integrated addrs, we don't want to extract the payment id and then use the integrated addr as well (TODO: unless we use fluffy's patch?)
		this->payment_id_string = boost::none;
		this->isXMRAddressIntegrated = true;
		this->integratedAddressPIDForDisplay = *decode_retVals.paymentID_string;
		this->_proceedTo_authOrSendTransaction();
		return;
	}
	// since we may have a payment ID here (which may also have been entered manually), validate
	if (monero_paymentID_utils::is_a_valid_or_not_a_payment_id(paymentID_toUseOrToNilIfIntegrated) == false) { // convenience function - will be true if nil pid
		this->parameters.failure_fn(enterValidPID, boost::none, boost::none, boost::none, boost::none);
		return;
	}
	if (paymentID_toUseOrToNilIfIntegrated != boost::none && paymentID_toUseOrToNilIfIntegrated->empty() == false) { // short pid / integrated address coersion
		if (decode_retVals.isSubaddress != true) { // this is critical or funds will be lost!!
			if (paymentID_toUseOrToNilIfIntegrated->size() == monero_paymentID_utils::payment_id_length__short) { // a short one
				THROW_WALLET_EXCEPTION_IF(decode_retVals.isSubaddress, error::wallet_internal_error, "Expected !decode_retVals.isSubaddress"); // just an extra safety measure
				optional<string> fabricated_integratedAddress_orNone = monero::address_utils::new_integratedAddrFromStdAddr( // construct integrated address
					*xmrAddress_toDecode, // the monero one
					*paymentID_toUseOrToNilIfIntegrated, // short pid
					this->parameters.nettype
				);
				if (fabricated_integratedAddress_orNone == boost::none) {
					this->parameters.failure_fn(couldntConstructIntAddrWithShortPid, boost::none, boost::none, boost::none, boost::none);
					return;
				}
				this->to_address_string = *fabricated_integratedAddress_orNone;
				this->payment_id_string = boost::none; // must now zero this or Send will throw a "pid must be blank with integrated addr"
				this->isXMRAddressIntegrated = true;
				this->integratedAddressPIDForDisplay = *paymentID_toUseOrToNilIfIntegrated; // a short pid
				this->_proceedTo_authOrSendTransaction();
				return; // return early to prevent fall-through to non-short or zero pid case
			}
		}
	}
	this->to_address_string = *xmrAddress_toDecode; // therefore, non-integrated normal XMR address
	this->payment_id_string = paymentID_toUseOrToNilIfIntegrated; // may still be nil
	this->isXMRAddressIntegrated = false;
	this->integratedAddressPIDForDisplay = boost::none;
	this->_proceedTo_authOrSendTransaction();
}
void FormSubmissionController::_proceedTo_authOrSendTransaction()
{
	if (this->parameters.requireAuthentication) {
		this->authenticate_fn(); // this will wait for authentication
	} else {
		this->_proceedTo_generateSendTransaction();
	}
}
void FormSubmissionController::_proceedTo_generateSendTransaction()
{
	// verify we were left with a good state
	THROW_WALLET_EXCEPTION_IF(this->to_address_string.size() == 0, error::wallet_internal_error, "Expected non-zero this->to_address_string");
	//
	this->parameters.preSuccess_passedValidation_willBeginSending();
	this->parameters.preSuccess_nonTerminal_validationMessageUpdate_fn(initiatingSend); // start with just prefix
	// ^--- TODO: this may be unnecessary .. there is no longer such a delay
	this->parameters.preSuccess_nonTerminal_validationMessageUpdate_fn(fetchingLatestBalance);
	auto req_params = new__req_params__get_unspent_outs(
		this->parameters.from_address_string,
		this->parameters.sec_viewKey_string
	);
	this->get_unspent_outs(req_params); // wait for cb I
}
void FormSubmissionController::cb__authentication(bool did_pass)
{
	if (did_pass == false) {
		this->parameters.canceled_fn();
		return;
	}
	this->_proceedTo_generateSendTransaction();
}
void FormSubmissionController::cb_I__got_unspent_outs(optional<string> err_msg, const optional<property_tree::ptree> &res)
{
	if (err_msg != boost::none && err_msg->empty() == false) {
		this->parameters.failure_fn(errGettingUnspentOuts_withMsg, err_msg.get(), boost::none, boost::none, boost::none);
		return;
	}
	crypto::secret_key sec_viewKey{};
	crypto::secret_key sec_spendKey{};
	crypto::public_key pub_spendKey{};
	{
		bool r = false;
		r = epee::string_tools::hex_to_pod(this->parameters.sec_viewKey_string, sec_viewKey);
		if (!r) {
			this->parameters.failure_fn(codeFault_invalidSecViewKey, boost::none, boost::none, boost::none, boost::none);
			return;
		}
		r = epee::string_tools::hex_to_pod(this->parameters.sec_spendKey_string, sec_spendKey);
		if (!r) {
			this->parameters.failure_fn(codeFault_invalidSecSpendKey, boost::none, boost::none, boost::none, boost::none);
			return;
		}
		r = epee::string_tools::hex_to_pod(this->parameters.pub_spendKey_string, pub_spendKey);
		if (!r) {
			this->parameters.failure_fn(codeFault_invalidPubSpendKey, boost::none, boost::none, boost::none, boost::none);
			return;
		}
	}
	auto parsed_res = new__parsed_res__get_unspent_outs(
		res.get(),
		sec_viewKey,
		sec_spendKey,
		pub_spendKey
	);
	if (parsed_res.err_msg != boost::none) {
		this->parameters.failure_fn(msgProvided, std::move(*(parsed_res.err_msg)), boost::none, boost::none, boost::none);
		return;
	}
	this->unspent_outs = std::move(*(parsed_res.unspent_outs));
	this->fee_per_b = *(parsed_res.per_byte_fee);
	this->fee_mask = *(parsed_res.fee_mask);
	//
	this->passedIn_attemptAt_fee = boost::none;
	this->constructionAttempt = 0;
	//
	_reenterable_construct_and_send_tx();
}
void FormSubmissionController::_reenterable_construct_and_send_tx()
{
	this->parameters.preSuccess_nonTerminal_validationMessageUpdate_fn(calculatingFee);
	//
	Send_Step1_RetVals step1_retVals;
	monero_transfer_utils::send_step1__prepare_params_for_get_decoys(
		step1_retVals,
		//
		this->payment_id_string,
		this->sending_amount,
		this->parameters.is_sweeping,
		this->parameters.priority,
		[] (uint8_t version, int64_t early_blocks) -> bool
		{
			return lightwallet_hardcoded__use_fork_rules(version, early_blocks);
		},
		this->unspent_outs,
		this->fee_per_b,
		this->fee_mask,
		//
		this->passedIn_attemptAt_fee // use this for passing step2 "must-reconstruct" return values back in, i.e. re-entry; when none, defaults to attempt at network min
		// ^- and this will be 'none' as initial value
	);
	if (step1_retVals.errCode != noError) {
		this->parameters.failure_fn(createTransactionCode_balancesProvided, boost::none, step1_retVals.errCode, step1_retVals.spendable_balance, step1_retVals.required_balance);
		return;
	}
	THROW_WALLET_EXCEPTION_IF(this->valsState != WAIT_FOR_STEP1, error::wallet_internal_error, "Expected valsState of WAIT_FOR_STEP1"); // for addtl safety
	// now store step1_retVals for step2
	this->step1_retVals__final_total_wo_fee = step1_retVals.final_total_wo_fee;
	this->step1_retVals__using_fee = step1_retVals.using_fee;
	this->step1_retVals__change_amount = step1_retVals.change_amount;
	this->step1_retVals__mixin = step1_retVals.mixin;
	THROW_WALLET_EXCEPTION_IF(this->step1_retVals__using_outs.size() != 0, error::wallet_internal_error, "Expected 0 using_outs");
	this->step1_retVals__using_outs = std::move(step1_retVals.using_outs); // move structs from stack's vector to heap's vector
	this->valsState = WAIT_FOR_STEP2;
	//
	this->parameters.preSuccess_nonTerminal_validationMessageUpdate_fn(fetchingDecoyOutputs);
	//
	auto req_params = new__req_params__get_random_outs(this->step1_retVals__using_outs); // use the one on the heap, since we've moved the one from step1_retVals
	this->get_random_outs(req_params); // wait for cb II
}
void FormSubmissionController::cb_II__got_random_outs(
	optional<string> err_msg,
	const optional<property_tree::ptree> &res
) {
	if (err_msg != boost::none && err_msg->empty() == false) {
		this->parameters.failure_fn(errGettingUnspentOuts_withMsg, err_msg.get(), boost::none, boost::none, boost::none);
		return;
	}
	auto parsed_res = new__parsed_res__get_random_outs(res.get());
	if (parsed_res.err_msg != boost::none) {
		this->parameters.failure_fn(msgProvided, std::move(*(parsed_res.err_msg)), boost::none, boost::none, boost::none);
		return;
	}
	THROW_WALLET_EXCEPTION_IF(this->step1_retVals__using_outs.size() == 0, error::wallet_internal_error, "Expected non-0 using_outs");
	Send_Step2_RetVals step2_retVals;
	uint64_t unlock_time = 0; // hard-coded for now since we don't ever expose it, presently
	monero_transfer_utils::send_step2__try_create_transaction(
		step2_retVals,
		//
		this->parameters.from_address_string,
		this->parameters.sec_viewKey_string,
		this->parameters.sec_spendKey_string,
		this->to_address_string,
		this->payment_id_string,
		*(this->step1_retVals__final_total_wo_fee),
		*(this->step1_retVals__change_amount),
		*(this->step1_retVals__using_fee),
		this->parameters.priority,
		this->step1_retVals__using_outs,
		this->fee_per_b,
		this->fee_mask,
		*(parsed_res.mix_outs),
		[] (uint8_t version, int64_t early_blocks) -> bool
		{
		return lightwallet_hardcoded__use_fork_rules(version, early_blocks);
		},
		unlock_time,
		this->parameters.nettype
	);
	if (step2_retVals.errCode != noError) {
		this->parameters.failure_fn(createTranasctionCode_noBalances, boost::none, step2_retVals.errCode, boost::none, boost::none);
		return;
	}
	if (step2_retVals.tx_must_be_reconstructed) {
		// this will update status back to .calculatingFee
		if (this->constructionAttempt > 15) { // just going to avoid an infinite loop here or particularly long stack
			this->parameters.failure_fn(exceededConstructionAttempts, boost::none, boost::none, boost::none, boost::none); // Unable to construct a transaction with sufficient fee for unknown reason
			return;
		}
		this->valsState = WAIT_FOR_STEP1; // must reset this
		//
		this->constructionAttempt += 1; // increment for re-entry
		this->passedIn_attemptAt_fee = step2_retVals.fee_actually_needed; // -> reconstruction attempt's step1's passedIn_attemptAt_fee
		// reset step1 vals for correctness: (otherwise we end up, for example, with duplicate outs added)
		this->step1_retVals__final_total_wo_fee = none;
		this->step1_retVals__change_amount = none;
		this->step1_retVals__using_fee = none;
		this->step1_retVals__mixin = none;
		this->step1_retVals__using_outs.clear(); // critical!
		// and let's reset step2 just for clarity/explicitness, though we don't expect them to have values yet:
		this->step2_retVals__signed_serialized_tx_string = boost::none;
		this->step2_retVals__tx_hash_string = boost::none;
		this->step2_retVals__tx_key_string = boost::none;
		this->step2_retVals__tx_pub_key_string = boost::none;
		//
		_reenterable_construct_and_send_tx();
		return;
	}
	THROW_WALLET_EXCEPTION_IF(this->valsState != WAIT_FOR_STEP2, error::wallet_internal_error, "Expected valsState of WAIT_FOR_STEP2"); // just for addtl safety
	// move step2 vals onto heap for later:
	this->step2_retVals__signed_serialized_tx_string = *(step2_retVals.signed_serialized_tx_string);
	this->step2_retVals__tx_hash_string = *(step2_retVals.tx_hash_string);
	this->step2_retVals__tx_key_string = *(step2_retVals.tx_key_string);
	this->step2_retVals__tx_pub_key_string = *(step2_retVals.tx_pub_key_string);
	//
	this->valsState = WAIT_FOR_FINISH;
	//
	this->parameters.preSuccess_nonTerminal_validationMessageUpdate_fn(submittingTransaction);
	//
	auto req_params = LightwalletAPI_Req_SubmitRawTx{
		this->parameters.from_address_string,
		this->parameters.sec_viewKey_string,
		*(step2_retVals.signed_serialized_tx_string)
	};
	this->submit_raw_tx(req_params); // wait for cb III
}
void FormSubmissionController::cb_III__submitted_tx(optional<string> err_msg)
{
	if (err_msg != boost::none && err_msg->empty() == false) {
		this->parameters.failure_fn(errGettingUnspentOuts_withMsg, err_msg.get(), boost::none, boost::none, boost::none);
		return;
	}
	THROW_WALLET_EXCEPTION_IF(this->valsState != WAIT_FOR_FINISH, error::wallet_internal_error, "Expected valsState of WAIT_FOR_FINISH"); // just for addtl safety
	// not actually expecting anything in a success response, so no need to parse it
	//
	Success_RetVals success_retVals;
	success_retVals.used_fee = *(this->step1_retVals__using_fee); // NOTE: not the same thing as step2_retVals.fee_actually_needed
	success_retVals.total_sent = *(this->step1_retVals__final_total_wo_fee) + *(this->step1_retVals__using_fee);
	success_retVals.mixin = *(this->step1_retVals__mixin);
	{
		optional<string> returning__payment_id = this->payment_id_string; // separated from submit_raw_tx_fn so that it can be captured w/o capturing all of args
		if (returning__payment_id == boost::none) {
			auto decoded = monero::address_utils::decodedAddress(this->to_address_string, this->parameters.nettype);
			if (decoded.did_error) { // would be very strange...
				this->parameters.failure_fn(couldntValidateDestAddress, boost::none, boost::none, boost::none, boost::none);
				return;
			}
			if (decoded.paymentID_string != boost::none) {
				returning__payment_id = std::move(*(decoded.paymentID_string)); // just preserving this as an original return value - this can probably eventually be removed
			}
		}
		success_retVals.final_payment_id = returning__payment_id;
	}
	success_retVals.signed_serialized_tx_string = *(this->step2_retVals__signed_serialized_tx_string);
	success_retVals.tx_hash_string = *(this->step2_retVals__tx_hash_string);
	success_retVals.tx_key_string = *(this->step2_retVals__tx_key_string);
	success_retVals.tx_pub_key_string = *(this->step2_retVals__tx_pub_key_string);
	success_retVals.final_total_wo_fee = *(this->step1_retVals__final_total_wo_fee);
	success_retVals.isXMRAddressIntegrated = this-isXMRAddressIntegrated;
	success_retVals.integratedAddressPIDForDisplay = this->integratedAddressPIDForDisplay;
	success_retVals.target_address = this->to_address_string;
	//
	this->parameters.success_fn(success_retVals);
}

// TODO: add another callback for this: there should be a way to cancel authorization as well from JS -> C++ ... that shoul cause the form submission controller mgr to release the form submission ctrlr
//
//	if SettingsController.shared.authentication__requireWhenSending == false {
//		__really_proceed()
//	} else {
//		PasswordController.shared.initiate_verifyUserAuthenticationForAction(
//																			 customNavigationBarTitle: NSLocalizedString("Authenticate", comment: ""),
//																			 canceled_fn: {
//																				 canceled_fn()
//																			 },
//																			 // all failures show in entry UI
//																			 entryAttempt_succeeded_fn: {
//																				 __really_proceed()
//																			 }
//																			 )
//	}

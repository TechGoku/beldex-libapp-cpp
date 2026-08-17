//
//  SendFundsFormSubmissionController.hpp
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

#ifndef SendFundsFormSubmissionController_hpp
#define SendFundsFormSubmissionController_hpp

#include <string>
#include <memory>
#include <boost/optional/optional.hpp>
#include <boost/locale.hpp>
#include "cryptonote_config.h"
#include "beldex_send_routine.hpp"
#include "beldex_fork_rules.hpp"

namespace SendFunds
{
	using namespace std;
	using namespace boost;
	using namespace boost::locale;
	using namespace cryptonote;
	using namespace beldex_send_routine;
	using namespace beldex_transfer_utils;
	//
	// Accessory Types
	enum ProcessStep
	{
		none = 0,
		initiatingSend = 1, // look at isSweeping for whether or not to display amount sent in message
		fetchingLatestBalance = 2,
		calculatingFee = 3,
		fetchingDecoyOutputs = 4,
		constructingTransaction = 5, // may go back to .calculatingFee
		submittingTransaction = 6
	};
	enum PreSuccessTerminalCode
	{
		msgProvided = 0, // use the optional provided string
		unableToLoadWallet = 1,
		unableToLogIntoWallet = 2,
		walletMustBeImported = 3,
		pleaseSpecifyRecipient = 4,
		couldntResolveThisOAAddress = 5,
		couldntValidateDestAddress = 6,
		enterValidPID = 7,
		couldntConstructIntAddrWithShortPid = 8,
		amountTooLow = 9,
		cannotParseAmount = 10,
		errInServerResponse_withMsg = 11,
		createTransactionCode_balancesProvided = 12,
		createTranasctionCode_noBalances = 13,
		exceededConstructionAttempts = 14, // unable to construct for unknown reason
		withSweepingOnlyOneAddressAllowed  = 15,
		numAmountsDoesntMatchNumRecipients = 16,
		invalidMNData=17,
		//
		codeFault_manualPaymentID_while_hasPickedAContact = 99900,
		codeFault_unableToFindResolvedAddrOnOAContact = 99901,
		codeFault_detectedPIDVisibleWhileManualInputVisible = 99902,
		codeFault_invalidSecViewKey = 99903, // considered a code fault since wallet should have validated
		codeFault_invalidSecSpendKey = 99904, // considered a code fault since wallet should have validated
		codeFault_invalidPubSpendKey = 99905, // considered a code fault since wallet should have validated
	};
	enum _Send_Task_ValsState
	{
		WAIT_FOR_HANDLE,
		WAIT_FOR_STEP1,
		WAIT_FOR_STEP2,
		WAIT_FOR_FINISH
	};
	struct Success_RetVals
	{
		vector<string> target_addresses; // this may differ from enteredAddress.. e.g. std addr + short pid -> int addr
		uint64_t used_fee;
		uint64_t final_total_wo_fee;
		uint64_t total_sent; // final_total_wo_fee + final_fee
		size_t mixin;
		bool isXMRAddressIntegrated; // regarding sentTo_address
		boost::optional<string> final_payment_id; // will be filled if a payment id was passed in or an integrated address was used
		boost::optional<string> integratedAddressPIDForDisplay;
		string signed_serialized_tx_string;
		string tx_hash_string;
		string tx_key_string; // this includes additional_tx_keys
		string tx_pub_key_string; // from get_tx_pub_key_from_extra()
		//! HF21: set only when this send deployed a new asset. It is the id the
		//! token now has on chain, and the only place the caller can learn it --
		//! it is derived from the descriptor, not chosen, and nothing else in the
		//! response identifies the token that was just created.
		boost::optional<string> token_id;
	};
	struct Parameters
	{
		//
		// Input values:
		boost::optional<master_node_data> mn_data;
		bool fromWallet_didFailToInitialize;
		bool fromWallet_didFailToBoot;
		bool fromWallet_needsImport;
		//
		bool requireAuthentication;
		//
		vector<string> send_amount_strings;
		bool is_sweeping;
		uint32_t priority;
		//
		bool hasPickedAContact;
		boost::optional<string> contact_payment_id;
		boost::optional<bool> contact_hasOpenAliasAddress;
		boost::optional<string> cached_OAResolved_address; // this may be an XMR address or a BTC address or … etc
		boost::optional<string> contact_address; // instead of the OAResolved_address
		//
		network_type nettype;
		string from_address_string;
		string sec_viewKey_string;
		string sec_spendKey_string;
		string pub_spendKey_string;
		//
		vector<string> enteredAddressValues;
		//
		boost::optional<string> resolvedAddress;
		bool resolvedAddress_fieldIsVisible;
		//
		boost::optional<string> manuallyEnteredPaymentID;
		bool manuallyEnteredPaymentID_fieldIsVisible;
		//
		boost::optional<string> resolvedPaymentID;
		bool resolvedPaymentID_fieldIsVisible;
		//
		// Process callbacks
		std::function<void(ProcessStep step)> preSuccess_nonTerminal_validationMessageUpdate_fn;
		std::function<void(
			PreSuccessTerminalCode code,
			boost::optional<string> msg,
			boost::optional<CreateTransactionErrorCode> createTx_errCode,
			boost::optional<uint64_t> spendable_balance,
			boost::optional<uint64_t> required_balance
		)> failure_fn;
		std::function<void(void)> preSuccess_passedValidation_willBeginSending; // use this to lock sending, pause idle timer, etc
		//
		std::function<void(void)> canceled_fn;
		std::function<void(Success_RetVals retVals)> success_fn;
		//
		// ── HF21 private tokens ───────────────────────────────────────────
		// Appended at the END on purpose: Parameters is built by positional
		// aggregate initialisation in beldex-libapp-js's emscr_SendFunds_bridge.cpp,
		// so inserting anywhere earlier silently re-binds every later field.
		// Trailing members are value-initialised by an initialiser that omits
		// them, which for boost::optional is `none` -- i.e. "ordinary BDX send".
		//
		//! Hex token id to send instead of BDX. The amounts in
		//! send_amount_strings are then denominated in this token, while the fee
		//! is still paid in BDX out of the wallet's native outputs.
		boost::optional<string> token_id;
		//! Decimal places of that token, from its descriptor (daemon
		//! get_token_info). Required whenever token_id is set: send_amount_strings
		//! are human-readable and a token's scale is its own, not BDX's 9, so
		//! parsing with the BDX scale would silently send the wrong quantity.
		boost::optional<uint8_t> token_decimal_point;
		//! Set to deploy a new asset rather than transfer an existing one. The
		//! send then creates the token described here, mints its initial supply
		//! to the wallet's own address, and burns the protocol's deployment fee
		//! in BDX on top of the network fee. token_id above must be the id
		//! derived from this descriptor, and token_decimal_point its decimals,
		//! so that the initial supply is parsed and tagged on the token's scale.
		boost::optional<token_operation_data> token_operation;
	};
	//
	// Controllers
	class FormSubmissionController
	{
	public:
		//
		// Lifecycle - Init
		FormSubmissionController(Parameters parameters)
		{
			this->parameters = parameters;
			this->valsState = WAIT_FOR_HANDLE;
		}
		//
		// Constructor args
		Parameters parameters;
		//
		// Remaining initialization args
		std::function<void(LightwalletAPI_Req_GetUnspentOuts req_params)> get_unspent_outs;
		std::function<void(LightwalletAPI_Req_GetRandomOuts req_params)> get_random_outs;
		std::function<void(LightwalletAPI_Req_SubmitRawTx req_params)> submit_raw_tx;
		std::function<void(void)> authenticate_fn;
		//
		// Imperatives - Initialization
		void set__get_unspent_outs_fn(std::function<void(LightwalletAPI_Req_GetUnspentOuts req_params)> fn);
		void set__get_random_outs_fn(std::function<void(LightwalletAPI_Req_GetRandomOuts req_params)> fn);
		void set__submit_raw_tx_fn(std::function<void(LightwalletAPI_Req_SubmitRawTx req_params)> fn);
		void set__authenticate_fn(std::function<void(void)> fn);
		//
		// Imperatives - Runtime
		void handle();
		void cb__authentication(bool did_pass/*false means canceled*/);
		void cb_I__got_unspent_outs(boost::optional<string> err_msg, const boost::optional<property_tree::ptree> &res);
		void cb_II__got_random_outs(boost::optional<string> err_msg, const boost::optional<property_tree::ptree> &res);
		void cb_III__submitted_tx(boost::optional<string> err_msg);
	private:
		//
		// Properties - Instance members
		// - state
		_Send_Task_ValsState valsState;
		// - from setup
		boost::optional<master_node_data> mn_data;
		vector<uint64_t> sending_amounts;
		vector<string> to_address_strings;
		boost::optional<string> payment_id_string;
		bool isXMRAddressIntegrated;
		boost::optional<string> integratedAddressPIDForDisplay;
		// - from cb_i
		vector<SpendableOutput> unspent_outs;
		uint64_t fee_per_b;
		uint64_t fee_per_o;
		uint64_t fee_mask;
		beldex_fork_rules::use_fork_rules_fn_type use_fork_rules;
		//! The same value use_fork_rules was built from, kept in raw form because
		//! transaction construction needs the number itself, not a predicate.
		uint8_t fork_version;
		// - re-entry params
		boost::optional<uint64_t> prior_attempt_size_calcd_fee;
		boost::optional<SpendableOutputToRandomAmountOutputs> prior_attempt_unspent_outs_to_mix_outs;
		size_t constructionAttempt;
		// - step1_retVals held for step2 - making them optl for increased safety
		boost::optional<uint64_t> step1_retVals__final_total_wo_fee;
		boost::optional<uint64_t> step1_retVals__change_amount;
		boost::optional<uint64_t> step1_retVals__using_fee;
		boost::optional<uint32_t> step1_retVals__mixin;
		vector<SpendableOutput> step1_retVals__using_outs;
		//! HF21+: token side of step1, carried to step2 alongside the BDX side.
		boost::optional<uint64_t> step1_retVals__token_final_total_wo_fee;
		boost::optional<uint64_t> step1_retVals__token_change_amount;
		// - step2_retVals held for submit tx - optl for increased safety
		boost::optional<string> step2_retVals__signed_serialized_tx_string;
		boost::optional<string> step2_retVals__tx_hash_string;
		boost::optional<string> step2_retVals__tx_key_string;
		boost::optional<string> step2_retVals__tx_pub_key_string;
		//
		// Imperatives
		void _proceedTo_authOrSendTransaction();
		void _proceedTo_generateSendTransaction();
		void _reenterable_construct_and_send_tx();
	};
}


#endif /* SendFundsFormSubmissionController_hpp */

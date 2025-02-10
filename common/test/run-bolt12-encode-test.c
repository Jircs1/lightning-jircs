/* Pipe through jq to create offers-test.json */
#include "config.h"
#include <assert.h>
#include <bitcoin/chainparams.h>
#include <ccan/array_size/array_size.h>
#include <ccan/tal/grab_file/grab_file.h>
#include <ccan/tal/path/path.h>
#include <ccan/tal/str/str.h>
#include <common/bech32_util.c>
#include <common/bech32.c>
#include <common/bigsize.c>
#include <common/bolt12.c>
#include <common/bolt12_merkle.h>
#include <common/features.c>
#include <common/sciddir_or_pubkey.c>
#include <common/setup.h>
#include <inttypes.h>
#include <wire/bolt12_wiregen.c>
#include <wire/fromwire.c>
#include <wire/onion_wiregen.c>
#include <wire/tlvstream.c>
#include <wire/towire.c>

/* AUTOGENERATED MOCKS START */
/* Generated stub for amount_asset_is_main */
bool amount_asset_is_main(struct amount_asset *asset UNNEEDED)
{ fprintf(stderr, "amount_asset_is_main called!\n"); abort(); }
/* Generated stub for amount_asset_to_sat */
struct amount_sat amount_asset_to_sat(struct amount_asset *asset UNNEEDED)
{ fprintf(stderr, "amount_asset_to_sat called!\n"); abort(); }
/* Generated stub for amount_feerate */
 bool amount_feerate(u32 *feerate UNNEEDED, struct amount_sat fee UNNEEDED, size_t weight UNNEEDED)
{ fprintf(stderr, "amount_feerate called!\n"); abort(); }
/* Generated stub for amount_sat */
struct amount_sat amount_sat(u64 satoshis UNNEEDED)
{ fprintf(stderr, "amount_sat called!\n"); abort(); }
/* Generated stub for amount_sat_add */
 bool amount_sat_add(struct amount_sat *val UNNEEDED,
				       struct amount_sat a UNNEEDED,
				       struct amount_sat b UNNEEDED)
{ fprintf(stderr, "amount_sat_add called!\n"); abort(); }
/* Generated stub for amount_sat_eq */
bool amount_sat_eq(struct amount_sat a UNNEEDED, struct amount_sat b UNNEEDED)
{ fprintf(stderr, "amount_sat_eq called!\n"); abort(); }
/* Generated stub for amount_sat_greater_eq */
bool amount_sat_greater_eq(struct amount_sat a UNNEEDED, struct amount_sat b UNNEEDED)
{ fprintf(stderr, "amount_sat_greater_eq called!\n"); abort(); }
/* Generated stub for amount_sat_sub */
 bool amount_sat_sub(struct amount_sat *val UNNEEDED,
				       struct amount_sat a UNNEEDED,
				       struct amount_sat b UNNEEDED)
{ fprintf(stderr, "amount_sat_sub called!\n"); abort(); }
/* Generated stub for amount_sat_to_asset */
struct amount_asset amount_sat_to_asset(struct amount_sat *sat UNNEEDED, const u8 *asset UNNEEDED)
{ fprintf(stderr, "amount_sat_to_asset called!\n"); abort(); }
/* Generated stub for amount_tx_fee */
struct amount_sat amount_tx_fee(u32 fee_per_kw UNNEEDED, size_t weight UNNEEDED)
{ fprintf(stderr, "amount_tx_fee called!\n"); abort(); }
/* Generated stub for fromwire_amount_msat */
struct amount_msat fromwire_amount_msat(const u8 **cursor UNNEEDED, size_t *max UNNEEDED)
{ fprintf(stderr, "fromwire_amount_msat called!\n"); abort(); }
/* Generated stub for merkle_tlv */
void merkle_tlv(const struct tlv_field *fields UNNEEDED, struct sha256 *merkle UNNEEDED)
{ fprintf(stderr, "merkle_tlv called!\n"); abort(); }
/* Generated stub for pubkey_from_node_id */
bool pubkey_from_node_id(struct pubkey *key UNNEEDED, const struct node_id *id UNNEEDED)
{ fprintf(stderr, "pubkey_from_node_id called!\n"); abort(); }
/* Generated stub for sighash_from_merkle */
void sighash_from_merkle(const char *messagename UNNEEDED,
			 const char *fieldname UNNEEDED,
			 const struct sha256 *merkle UNNEEDED,
			 struct sha256 *sighash UNNEEDED)
{ fprintf(stderr, "sighash_from_merkle called!\n"); abort(); }
/* Generated stub for towire_amount_msat */
void towire_amount_msat(u8 **pptr UNNEEDED, const struct amount_msat msat UNNEEDED)
{ fprintf(stderr, "towire_amount_msat called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

static utf8 *tal_utf8(const tal_t *ctx, const char *str)
{
	/* Non-NUL terminated! */
	return tal_dup_arr(ctx, char, str, strlen(str), 0);
}

static void print_field(bool first, const struct tlv_field *field)
{
	printf("%s{\"type\": %"PRIu64", \"length\": %zu, \"hex\": \"%s\"}",
	       first ? "": ",",
	       field->numtype,
	       field->length,
	       tal_hexstr(tmpctx, field->value, field->length));
}

static void print_valid_offer(const struct tlv_offer *offer,
			      const char *testdesc,
			      const char *extradesc,
			      const struct tlv_field *extrafield)
{
	char *str = offer_encode(tmpctx, offer), *err;
	struct tlv_offer *offer2;

	/* We only use extrafield on the end */
	if (extrafield) {
		u8 *wire;

		wire = tal_arr(tmpctx, u8, 0);
		towire_tlv_offer(&wire, offer);
		towire_bigsize(&wire, extrafield->numtype);
		towire_bigsize(&wire, extrafield->length);
		towire(&wire, extrafield->value, extrafield->length);

		str = to_bech32_charset(tmpctx, "lno", wire);
	}

	printf("%s{ \"description\": \"%s\", \"valid\": true, \"bolt12\": \"%s\",",
	       extradesc ? "," : "",
	       testdesc, str);
	if (extradesc)
		printf("\"field info\": \"%s\", ", extradesc);

	/* Print the fields. */
	printf("\"fields\": [");
	/* Re-marshal to get fields[] arr (and sanity check!) */
	offer2 = offer_decode(tmpctx, str, strlen(str), NULL, NULL, &err);
	for (size_t i = 0; i < tal_count(offer2->fields); i++)
		print_field(i == 0, &offer2->fields[i]);
	printf("]}\n");
}

static void print_malformed_tlv(const char *hrp,
				const char *hex,
				const char *reason)
{
	u8 *data = tal_hexdata(tmpctx, hex, strlen(hex));
	assert(data);
	printf(",{\"description\": \"%s\", \"valid\": false, \"bolt12\": \"%s\"}",
	       reason, to_bech32_charset(tmpctx, hrp, data));
}

static void print_invalid_offer(const struct tlv_offer *offer, const char *why)
{
	printf(",{\"description\": \"%s\", \"valid\": false, \"bolt12\": \"%s\"}",
	       why, offer_encode(tmpctx, offer));
}

int main(int argc, char *argv[])
{
	struct tlv_offer *offer;
	struct secret alice, bob;
	struct blinded_path **paths;

	common_setup(argv[0]);

	memset(&alice, 'A', sizeof(alice));
	memset(&bob, 'B', sizeof(bob));

	offer = tlv_offer_new(tmpctx);
	offer->offer_issuer_id = tal(offer, struct pubkey);
	assert(pubkey_from_secret(&alice, offer->offer_issuer_id));

	printf("[\n");
	print_valid_offer(offer, "Minimal bolt12 offer", NULL, NULL);
	offer->offer_description = tal_utf8(tmpctx, "Test vectors");
	print_valid_offer(offer, "with description (but no amount)",
			  "description is 'Test vectors'", NULL);

	offer->offer_chains = tal_arr(offer, struct bitcoin_blkid, 1);
	offer->offer_chains[0] = chainparams_for_network("testnet")->genesis_blockhash;
	print_valid_offer(offer, "for testnet",
			  "chains[0] is testnet", NULL);
	offer->offer_chains[0] = chainparams_for_network("bitcoin")->genesis_blockhash;
	print_valid_offer(offer, "for bitcoin (redundant)",
			  "chains[0] is bitcoin", NULL);
	offer->offer_chains = tal_arr(offer, struct bitcoin_blkid, 2);
	offer->offer_chains[0] = chainparams_for_network("liquid")->genesis_blockhash;
	offer->offer_chains[1] = chainparams_for_network("bitcoin")->genesis_blockhash;
	print_valid_offer(offer, "for bitcoin or liquidv1",
			  "chains[0] is liquidv1, chains[1] is bitcoin", NULL);
	offer->offer_chains = NULL;

	offer->offer_metadata = tal_arrz(offer, u8, 16);
	print_valid_offer(offer, "with metadata",
			  "metadata is 16 zero bytes", NULL);
	offer->offer_metadata = NULL;

	offer->offer_amount = tal(offer, u64);
	*offer->offer_amount = 10000;
	print_valid_offer(offer, "with amount",
			  "amount is 10000msat", NULL);

	offer->offer_currency = tal_utf8(offer, "USD");
	print_valid_offer(offer, "with currency",
			  "amount is USD $100.00", NULL);
	offer->offer_currency = NULL;
	offer->offer_amount = NULL;

	offer->offer_absolute_expiry = tal(offer, u64);
	*offer->offer_absolute_expiry = 2051184600;
	print_valid_offer(offer, "with expiry",
			  "expiry is 2035-01-01", NULL);
	offer->offer_absolute_expiry = NULL;

	offer->offer_issuer = tal_utf8(offer, "https://bolt12.org BOLT12 industries");
	print_valid_offer(offer, "with issuer",
			  "issuer is 'https://bolt12.org BOLT12 industries'", NULL);
	offer->offer_issuer = NULL;

	offer->offer_quantity_max = tal(offer, u64);
	*offer->offer_quantity_max = 5;
	print_valid_offer(offer, "with quantity",
			  "quantity_max is 5", NULL);

	*offer->offer_quantity_max = 0;
	print_valid_offer(offer, "with unlimited (or unknown) quantity",
			  "quantity_max is unknown/unlimited", NULL);

	*offer->offer_quantity_max = 1;
	print_valid_offer(offer, "with single quantity (weird but valid)",
			  "quantity_max is 1", NULL);
	offer->offer_quantity_max = NULL;

	offer->offer_features = tal_arr(offer, u8, 0);
	set_feature_bit(&offer->offer_features, 99);
	print_valid_offer(offer, "with feature",
			  "feature bit 99 set", NULL);
	offer->offer_features = NULL;

	offer->offer_paths = tal_arr(offer, struct blinded_path *, 1);
	offer->offer_paths[0] = tal(offer->offer_paths, struct blinded_path);
	offer->offer_paths[0]->first_node_id.is_pubkey = true;
	assert(pubkey_from_secret(&bob, &offer->offer_paths[0]->first_node_id.pubkey));
	/* Random blinding secret. */
	assert(pubkey_from_hexstr("020202020202020202020202020202020202020202020202020202020202020202", 66, &offer->offer_paths[0]->first_path_key));
	offer->offer_paths[0]->path = tal_arr(offer->offer_paths[0],
					      struct blinded_path_hop *,
					      2);
	offer->offer_paths[0]->path[0] = tal(offer->offer_paths[0]->path,
					     struct blinded_path_hop);
	assert(pubkey_from_hexstr("020202020202020202020202020202020202020202020202020202020202020202", 66, &offer->offer_paths[0]->path[0]->blinded_node_id));
	offer->offer_paths[0]->path[0]->encrypted_recipient_data = tal_arrz(offer->offer_paths[0]->path[0], u8, 16);
	offer->offer_paths[0]->path[1] = tal(offer->offer_paths[0]->path,
					     struct blinded_path_hop);
	assert(pubkey_from_hexstr("020202020202020202020202020202020202020202020202020202020202020202", 66, &offer->offer_paths[0]->path[1]->blinded_node_id));
	offer->offer_paths[0]->path[1]->encrypted_recipient_data = tal_hexdata(offer->offer_paths[0]->path[1], "1111111111111111", 16);
	print_valid_offer(offer, "with blinded path via Bob (0x424242...), path_key 020202...",
			  "path is [id=02020202..., enc=0x00*16], [id=02020202..., enc=0x11*8]", NULL);

	/* BOLT #12:
	 *   - if it includes `offer_paths`:
	 *     - MAY set `offer_issuer_id`.
	 *   - otherwise:
	 *      - MUST set `offer_issuer_id` to the node's public key to request the invoice from.
	 */
	offer->offer_issuer_id = tal_free(offer->offer_issuer_id);
	print_valid_offer(offer, "with no issuer_id and blinded path via Bob (0x424242...), path_key 020202...",
			  "path is [id=02020202..., enc=0x00*16], [id=02020202..., enc=0x11*8]", NULL);
	offer->offer_issuer_id = tal(offer, struct pubkey);
	assert(pubkey_from_secret(&alice, offer->offer_issuer_id));

	tal_resize(&offer->offer_paths, 2);
	offer->offer_paths[1] = tal(offer->offer_paths, struct blinded_path);
	offer->offer_paths[1]->first_node_id.is_pubkey = false;
	assert(short_channel_id_dir_from_str("1x2x3/1", strlen("1x2x3/1"),
					     &offer->offer_paths[1]->first_node_id.scidd));
	/* Random path_key secret. */
	assert(pubkey_from_hexstr("020202020202020202020202020202020202020202020202020202020202020202", 66, &offer->offer_paths[1]->first_path_key));
	offer->offer_paths[1]->path = tal_arr(offer->offer_paths[1],
					      struct blinded_path_hop *,
					      2);
	offer->offer_paths[1]->path[0] = tal(offer->offer_paths[1]->path,
					     struct blinded_path_hop);
	assert(pubkey_from_hexstr("020202020202020202020202020202020202020202020202020202020202020202", 66, &offer->offer_paths[1]->path[0]->blinded_node_id));
	offer->offer_paths[1]->path[0]->encrypted_recipient_data = tal_arrz(offer->offer_paths[1]->path[0], u8, 16);
	offer->offer_paths[1]->path[1] = tal(offer->offer_paths[1]->path,
					     struct blinded_path_hop);
	assert(pubkey_from_hexstr("020202020202020202020202020202020202020202020202020202020202020202", 66, &offer->offer_paths[1]->path[1]->blinded_node_id));
	offer->offer_paths[1]->path[1]->encrypted_recipient_data = tal_hexdata(offer->offer_paths[1]->path[1], "2222222222222222", 16);
	print_valid_offer(offer, "... and with second blinded path via 1x2x3 (direction 1), path_key 020202...",
			  "path is [id=02020202..., enc=0x00*16], [id=02020202..., enc=0x22*8]", NULL);
	paths = offer->offer_paths;
	offer->offer_paths = NULL;

	/* Unknown odd fields are fine */
	struct tlv_field extra;
	extra.numtype = 33;
	extra.length = 10;
	extra.value = (u8 *)"helloworld";
	print_valid_offer(offer, "unknown odd field",
			  "type 33 is 'helloworld'", &extra);

	extra.numtype = 1000000033;
	extra.length = 10;
	extra.value = (u8 *)"helloworld";
	print_valid_offer(offer, "unknown odd experimental field",
			  "type 1000000033 is 'helloworld'", &extra);

	/* Now let's do the invalid ones! */

	/* Invalid encoding forms. */
	/* offer_issuer_id then description */
	print_malformed_tlv("lno",
			    "1621020202020202020202020202020202020202020202020202020202020202020202" /* offer_issuer_id */
			    "0A05414C494345", /* offer_description */
			    "Malformed: fields out of order");
	print_malformed_tlv("lno",
			    "0A05414C494345" /* offer_description */
			    "1621020202020202020202020202020202020202020202020202020202020202020202" /* offer_issuer_id */
			    "48206fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000",
			    "Malformed: unknown even TLV type 78");
	/* various forms of truncation */
	print_malformed_tlv("lno", "", "Malformed: empty");
	print_malformed_tlv("lno", "0A", "Malformed: truncated at type");
	print_malformed_tlv("lno", "0AFD", "Malformed: truncated in length");
	print_malformed_tlv("lno", "0A02", "Malformed: truncated after length");
	print_malformed_tlv("lno", "0A0241", "Malformed: truncated in description");

	print_malformed_tlv("lno",
			    "020101"
			    "0A05414C494345"
			    "1621020202020202020202020202020202020202020202020202020202020202020202",
			    "Malformed: invalid offer_chains length");

	print_malformed_tlv("lno",
			    "060180"
			    "0A05414C494345"
			    "1621020202020202020202020202020202020202020202020202020202020202020202",
			    "Malformed: truncated currency UTF-8");
	print_malformed_tlv("lno",
			    "06028041"
			    "0A05414C494345"
			    "1621020202020202020202020202020202020202020202020202020202020202020202",
			    "Malformed: invalid currency UTF-8");
	print_malformed_tlv("lno",
			    "0A0180"
			    "1621020202020202020202020202020202020202020202020202020202020202020202",
			    "Malformed: truncated description UTF-8");
	print_malformed_tlv("lno",
			    "0A028041"
			    "1621020202020202020202020202020202020202020202020202020202020202020202",
			    "Malformed: invalid description UTF-8");
	print_malformed_tlv("lno",
			    "0A05414C494345"
			    "100101"
			    "1621020202020202020202020202020202020202020202020202020202020202020202",
			    "Malformed: truncated offer_paths");
	print_malformed_tlv("lno",
			    "0A05414C494345"
			    "1002020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020200"
			    "1621020202020202020202020202020202020202020202020202020202020202020202",
			    "Malformed: zero num_hops in blinded_path");
	print_malformed_tlv("lno",
			    "0A05414C494345"
			    "100202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020102020202020202020202020202020202020202020202020202020202020202020201"
			    "1621020202020202020202020202020202020202020202020202020202020202020202",
			    "Malformed: truncated blinded_path_hop in blinded_path");
	print_malformed_tlv("lno",
			    "0A05414C494345"
			    "10030303030303030303030303030303030303030303030303030303030303030303020202020202020202020202020202020202020202020202020202020202020202010202020202020202020202020202020202020202020202020202020202020202020100"
			    "1621020202020202020202020202020202020202020202020202020202020202020202",
			    "Malformed: bad first_node_id in blinded_path");
	print_malformed_tlv("lno",
			    "0A05414C494345"
			    "10020202020202020202020202020202020202020202020202020202020202020202030303030303030303030303030303030303030303030303030303030303030303010202020202020202020202020202020202020202020202020202020202020202020100"
			    "1621020202020202020202020202020202020202020202020202020202020202020202",
			    "Malformed: bad path_key in blinded_path");
	print_malformed_tlv("lno",
			    "0A05414C494345"
			    "10020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202020202010303030303030303030303030303030303030303030303030303030303030303030100"
			    "1621020202020202020202020202020202020202020202020202020202020202020202",
			    "Malformed: bad blinded_node_id in blinded_path_hop");

	print_malformed_tlv("lno",
			    "0A05414C494345"
			    "120180"
			    "1621020202020202020202020202020202020202020202020202020202020202020202",
			    "Malformed: truncated issuer UTF-8");
	print_malformed_tlv("lno",
			    "0A05414C494345"
			    "12028041"
			    "1621020202020202020202020202020202020202020202020202020202020202020202",
			    "Malformed: invalid issuer UTF-8");
	print_malformed_tlv("lno",
			    "0A05414C494345"
			    "1621020303030303030303030303030303030303030303030303030303030303030303",
			    "Malformed: invalid offer_issuer_id");

	/* Now these are simply invalid, not bad encodings */
	/* BOLT #12:
	 * A reader of an offer:
	 *   - if the offer contains any TLV fields outside the inclusive ranges: 1 to 79 and 1000000000 to 1999999999:
	 *     - MUST NOT respond to the offer.
	 */
	print_malformed_tlv("lno",
			    "0A05414C494345" /* offer_description */
			    "1621020202020202020202020202020202020202020202020202020202020202020202" /* offer_issuer_id */
			    "50206fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000",
			    "Contains type >= 80");

	print_malformed_tlv("lno",
			    "0A05414C494345" /* offer_description */
			    "1621020202020202020202020202020202020202020202020202020202020202020202" /* offer_issuer_id */
			    "FD7735940050206fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000",
			    "Contains type > 1999999999");

	print_malformed_tlv("lno",
			    "0A05414C494345" /* offer_description */
			    "1621020202020202020202020202020202020202020202020202020202020202020202" /* offer_issuer_id */
			    "FD3B9ACA0250206fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000",
			    "Contains unknown even type (1000000002)");

	offer = tlv_offer_new(tmpctx);
	offer->offer_issuer_id = tal(offer, struct pubkey);
	assert(pubkey_from_secret(&alice, offer->offer_issuer_id));
	offer->offer_description = tal_utf8(tmpctx, "Test vectors");

	offer->offer_features = tal_arr(offer, u8, 0);
	set_feature_bit(&offer->offer_features, 22);
	/* BOLT #12:
	 *   - if `offer_features` contains unknown _even_ bits that are non-zero:
	 *     - MUST NOT respond to the offer.
	 *     - SHOULD indicate the unknown bit to the user.
	 */
	print_invalid_offer(offer, "Contains unknown feature 22");
	offer->offer_features = NULL;

	/* BOLT #12:
	 *   - if `offer_amount` is set and `offer_description` is not set:
	 *     - MUST NOT respond to the offer.
	 *   - if `offer_currency` is set and `offer_amount` is not set:
	 *     - MUST NOT respond to the offer.
	 *   - if neither `offer_issuer_id` nor `offer_paths` are set:
	 *     - MUST NOT respond to the offer.
	 */
	offer->offer_description = NULL;
	print_invalid_offer(offer, "Missing offer_description and offer_amount");
	offer->offer_description = tal_utf8(tmpctx, "Test vectors");

	offer->offer_currency = tal_utf8(offer, "USD");
	print_invalid_offer(offer, "Missing offer_amount with offer_currency");
	offer->offer_currency = NULL;

	offer->offer_issuer_id = NULL;
	print_invalid_offer(offer, "Missing offer_issuer_id and no offer_path");

	offer->offer_paths = paths;
	offer->offer_paths[1]->path = NULL;
	print_invalid_offer(offer, "Second offer_path is empty");

	printf("]\n");
	common_shutdown();
}

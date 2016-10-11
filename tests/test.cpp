#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include <stdexcept>

extern "C" {
#include <utils_string.h>
#include <cafe_shell.h>
#include <tree.h>
#include <cafe.h>
};

#include <cafe_commands.h>
#include "lambda.h"

extern "C" {
	void show_sizes(FILE*, pCafeParam param, pCafeFamilyItem pitem, int i);
	void phylogeny_lambda_parse_func(pTree ptree, pTreeNode ptnode);
}


static void init_cafe_tree()
{
	const char *newick_tree = "(((chimp:6,human:6):81,(mouse:17,rat:17):70):6,dog:9)";
	char buf[100];
	strcpy(buf, "tree ");
	strcat(buf, newick_tree);
	cafe_shell_dispatch_command(buf);
}


int cafe_cmd_atoi(int argc, char* argv[])
{
	return atoi(argv[1]);
}

pArrayList build_arraylist(const char *items[], int count)
{
	pArrayList psplit = arraylist_new(20);
	for (int i = 0; i < count; ++i)
	{
		char *str = (char*)memory_new(strlen(items[i]) + 1, sizeof(char));
		strcpy(str, items[i]);
		arraylist_add(psplit, str);
	}
	return psplit;
}


CafeShellCommand cafe_cmd_test[] =
{
	{ "atoi", cafe_cmd_atoi },
	{ NULL, NULL }
};


TEST_GROUP(FirstTestGroup)
{
};

TEST(FirstTestGroup, TestStringSplitter)
{
	char c[10];
	pArrayList pArray;
	strcpy(c, "a b");
	pArray = string_pchar_space_split(c);
	LONGS_EQUAL(2, pArray->size);
	STRCMP_EQUAL("a", (char *)(pArray->array[0]));
	STRCMP_EQUAL("b", (char *)(pArray->array[1]));
}

TEST(FirstTestGroup, TestCafeFamilyNew)
{
	pCafeFamily fam;
	char fname[100];
	strcpy(fname, "Nonexistent.tab");
	POINTERS_EQUAL(NULL, cafe_family_new(fname, 1));

	strcpy(fname, "../example/example_data.tab");
	fam = cafe_family_new(fname, 1);
	LONGS_EQUAL(5, fam->num_species);
	STRCMP_EQUAL("Dog", fam->species[0]);
	LONGS_EQUAL(59, fam->flist->size);
	STRCMP_EQUAL("Rat", fam->species[4]);
	LONGS_EQUAL(59, fam->flist->size);
}

TEST(FirstTestGroup, TestCafeTree)
{
	const char *newick_tree = "(((chimp:6,human:6):81,(mouse:17,rat:17):70):6,dog:9)";
	char tree[100];
	strcpy(tree, newick_tree);
	int family_sizes[2] = { 1,1 };
	int rootfamily_sizes[2] = { 1,1 };
	pCafeTree cafe_tree = cafe_tree_new(tree, family_sizes, rootfamily_sizes, 0, 0);
	LONGS_EQUAL(128, cafe_tree->super.size);

	// Find chimp in the tree after two branches of length 6,81,6
	pTreeNode ptnode = (pTreeNode)cafe_tree->super.root;
	CHECK(tree_is_root(&cafe_tree->super, ptnode));
	ptnode = (pTreeNode)tree_get_child(ptnode, 0);
	pPhylogenyNode pnode = (pPhylogenyNode)ptnode;
	LONGS_EQUAL(6, pnode->branchlength);	// root to first branch = 6

	ptnode = (pTreeNode)tree_get_child(ptnode, 0);
	pnode = (pPhylogenyNode)ptnode;
	LONGS_EQUAL(81, pnode->branchlength); // 1st to 2nd = 81

	ptnode = (pTreeNode)tree_get_child(ptnode, 0);
	pnode = (pPhylogenyNode)ptnode;
	STRCMP_EQUAL("chimp", pnode->name);
	LONGS_EQUAL(6, pnode->branchlength);  // 2nd to chimp leaf = 6
	CHECK(tree_is_leaf(ptnode));


}

TEST(FirstTestGroup, TestShellDispatcher)
{
	char c[10];
	cafe_shell_init(1);

	CafeShellCommand *old = cafe_cmd;
	cafe_cmd[0] = cafe_cmd_test[0];
	cafe_cmd[1] = cafe_cmd_test[1];

	strcpy(c, "atoi 9528");
	LONGS_EQUAL(9528, cafe_shell_dispatch_command(c));

	strcpy(c, "# a comment");
	LONGS_EQUAL(0, cafe_shell_dispatch_command(c));

	strcpy(c, "unknown");
	LONGS_EQUAL(CAFE_SHELL_NO_COMMAND, cafe_shell_dispatch_command(c));
}

TEST(FirstTestGroup, TestShowSizes)
{
	char outbuf[10000];
	setbuf(stdout, outbuf);

	CafeParam param;
	param.rootfamily_sizes[0] = 29;
	param.rootfamily_sizes[1] = 31;
	param.family_sizes[0] = 37;
	param.family_sizes[1] = 41;

	CafeFamilyItem item;
	item.ref = 14;
	CafeTree tree;
	tree.rootfamilysizes[0] = 11;
	tree.rootfamilysizes[1] = 13;
	tree.familysizes[0] = 23;
	tree.familysizes[1] = 19;
	tree.rfsize = 17;
	param.pcafe = &tree;
	FILE* in = fmemopen(outbuf, 999, "w");
	show_sizes(in, &param, &item, 7);
	fclose(in);
	STRCMP_CONTAINS(">> 7 14", outbuf);
	STRCMP_CONTAINS("Root size: 11 ~ 13 , 17", outbuf);
	STRCMP_CONTAINS("Family size: 23 ~ 19", outbuf);
	STRCMP_CONTAINS("Root size: 29 ~ 31", outbuf);
	STRCMP_CONTAINS("Family size: 37 ~ 41", outbuf);
}

TEST(FirstTestGroup, TestPhylogenyLoadFromString)
{
	char outbuf[10000];
	strcpy(outbuf, "(((1,1)1,(2,2)2)2,2)");
	cafe_shell_init(1);
	init_cafe_tree();
	pTree tree = phylogeny_load_from_string(outbuf, tree_new, phylogeny_new_empty_node, phylogeny_lambda_parse_func, 0);
	CHECK(tree != 0);
	LONGS_EQUAL(56, tree->size);

};

TEST(FirstTestGroup, Test_cafe_cmd_source)
{
	std::vector<std::string> strs;
	strs.push_back("source");
	LONGS_EQUAL( -1, cafe_cmd_source(strs));

	strs.push_back("nonexistent");
	LONGS_EQUAL(-1, cafe_cmd_source(strs));
};

TEST(FirstTestGroup, Test_cafe_get_posterior)
{
	CafeParam param;
	CafeFamily family;
	param.pfamily = &family;
	//cafe_get_posterior(&param);
};

TEST(FirstTestGroup, cafe_family_set_size)
{
	CafeFamily family;
	CafeTree tree;
	tree.size_of_factor = 0;
	family.flist = arraylist_new(11000);
	pCafeFamilyItem pitem = (pCafeFamilyItem)memory_new(1, sizeof(CafeFamilyItem));
	arraylist_add(family.flist, pitem);
	printf("Tree size: %d\n", tree.super.size);
	cafe_tree_new_empty_node((pTree)&tree);
	//cafe_family_set_size(&family, 0, &tree);
	//pCafeNode pcnode = (pCafeNode)tree.super.nlist->array[0];
	//LONGS_EQUAL( pcnode->familysize, 5);

};

TEST(FirstTestGroup, cafe_family_init)
{
	const char *species[] = { "", "", "chimp", "human", "mouse", "rat", "dog" };
	pArrayList psplit = build_arraylist(species, 7);

	pCafeFamily pcf = cafe_family_init(psplit);
	arraylist_free(psplit, NULL);
	LONGS_EQUAL(5, pcf->num_species);
	STRCMP_EQUAL("dog", pcf->species[4]);
	STRCMP_EQUAL("rat", pcf->species[3]);
	LONGS_EQUAL(5, pcf->num_species);
	LONGS_EQUAL(0, pcf->max_size);
	LONGS_EQUAL(0, pcf->flist->size);
	LONGS_EQUAL(-1, pcf->index[1]);
	LONGS_EQUAL(-1, pcf->index[2]);

}

TEST(FirstTestGroup, cafe_family_add_item)
{
	const char *species[] = { "", "", "chimp", "human", "mouse", "rat", "dog" };

	pArrayList psplit = build_arraylist(species, 7);
	pCafeFamily pcf = cafe_family_init(psplit);

	const char *values[] = { "description", "id", "3", "5", "7", "11", "13" };
	cafe_family_add_item(pcf, build_arraylist(values, 7));
	arraylist_free(psplit, NULL);
	LONGS_EQUAL(1, pcf->flist->size);
	pCafeFamilyItem pitem = (pCafeFamilyItem)arraylist_get(pcf->flist, 0);
	STRCMP_EQUAL("description", pitem->desc);
	STRCMP_EQUAL("id", pitem->id);
	LONGS_EQUAL(3, pitem->count[0]);
	LONGS_EQUAL(5, pitem->count[1]);
	LONGS_EQUAL(7, pitem->count[2]);
	LONGS_EQUAL(11, pitem->count[3]);
	LONGS_EQUAL(13, pitem->count[4]);
	LONGS_EQUAL(pitem->maxlh,-1);
	LONGS_EQUAL(pitem->ref,-1);
	LONGS_EQUAL(pitem->lambda, NULL);
	LONGS_EQUAL(pitem->mu, NULL);
	LONGS_EQUAL(pitem->pbdc_array, NULL);
	LONGS_EQUAL(pitem->holder, 1);
	LONGS_EQUAL(13, pcf->max_size);

}

int main(int ac, char** av)
{
	return CommandLineTestRunner::RunAllTests(ac, av);
}

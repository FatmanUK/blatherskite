#include <iostream>
#include <memory>
#include <map>

#include "api.hh"

using std::string;
using std::cerr;
using std::endl;
using std::unique_ptr;
using std::shared_ptr;
using std::map;

const char IMG_NONE =	0;
const char IMG_LOCAL =	1;
const char IMG_TRASH =	2;
const char IMG_OUTBOX =	3;
const char IMG_INBOX =	4;
const char IMG_EMAIL =	5;
const char IMG_DRAFTS =	6;
const char IMG_NOTES =	7;

const string PNAME = "mailbox";
const string PVERSION = "v0.0.2";
const string TABNAME = "E-&Mail";

const string TREE_ROOT = "E-mail Accounts";

void *tab{nullptr};
map<char, void *> images;

/*

****__________________
|    |                |
|box |      list      |
|    |                |
|    |----------------|
|    |                |
|    |      read      |
|____|________________|
|_______status________|
*/

//void cb_btn_test(void *, void *) {
//	cerr << "Mail test!" << endl;
//}

//	add_html_pane(tab);

/*
void add_button(void *grp, cb_fn_ptr fn, const string &label) {
	// convert group to pack, first
	// TO DO: detect if group first?
	auto pck = reinterpret_cast<Fl_Group *>(grp)->array()[0];
	auto btn = new Fl_Button(20, 20, 70, 40, label.c_str());
	btn->callback(reinterpret_cast<fltk_cb_fn>(fn), nullptr);
	reinterpret_cast<Fl_Pack *>(pck)->add(btn);
}
*/

//void *add_vpack(void *ptr) {
//
//}

// FLTK Fl_Widgets *children* are auto-deleted so do not need to be
// memory managed.
// Watch out for Fl_Image which is NOT managed by FLTK and so must be
// deleted manually.

bool start(void *ui) {
	string images_dir{XSTR(IMAGES_DIR)};
	images_dir = expand_bash_tilde(images_dir);
#ifndef NDEBUG
	cerr << "Looking for images in: " << images_dir << endl;
#endif
	cerr << "Loading: " << PNAME << " " << PVERSION << "" << endl;
#ifndef NDEBUG
	cerr << "DEBUG build" << endl;
#endif
	bool is_dir = path_is_extant_dir(images_dir);
	THROW_IF_FALSE(is_dir, "Images dir " + images_dir + " not found");
	bool can_read = dir_allows_read(images_dir);
	THROW_IF_FALSE(can_read, "Image dir " + images_dir + " not readable");
	// Load images.
	images[IMG_LOCAL] = load_image(images_dir + "/computer.png");
	images[IMG_TRASH] = load_image(images_dir + "/trash.png");
	images[IMG_OUTBOX] = load_image(images_dir + "/outbox.png");
	images[IMG_INBOX] = load_image(images_dir + "/inbox.png");
	images[IMG_EMAIL] = load_image(images_dir + "/email.png");
	images[IMG_DRAFTS] = load_image(images_dir + "/drafts.png");
	images[IMG_NOTES] = load_image(images_dir + "/notes.png");
	// Set up UI.
	tab = create_tab(ui, TABNAME);
	void *vp = add_vpack(tab);
	void *boxes = add_tree(vp, TREE_ROOT);
	void *root = get_tree_root(boxes);

	void *local = add_treeitem(ui, boxes, root, "Local Folders", images[IMG_LOCAL]);
	void *local_trash = add_treeitem(ui, boxes, local, "Trash", images[IMG_TRASH]);
	void *local_outbox = add_treeitem(ui, boxes, local, "Outbox", images[IMG_OUTBOX]);

	void *acct1 = add_treeitem(ui, boxes, root, "fatman@dreamtrack.co.uk", images[IMG_EMAIL]);
	void *acct1_inbox = add_treeitem(ui, boxes, acct1, "Inbox", images[IMG_INBOX]);
	void *acct1_drafts = add_treeitem(ui, boxes, acct1, "Drafts", images[IMG_DRAFTS]);
	void *acct1_trash = add_treeitem(ui, boxes, acct1, "Trash", images[IMG_TRASH]);
	void *acct1_notes = add_treeitem(ui, boxes, acct1, "Notes", images[IMG_NOTES]);

/*
//	add_button(ui, TABNAME, &cb_btn_test, BTN_MAIL_NAME);
//	add_button(ui, TABNAME, &cb_btn_test, BTN_MAIL_NAME2);
//	void *sb = add_status_bar(tab); // online/offline f@d.c.u: Looked up mail.d.c.u... [50%] Unread 1 Total 4927

	void *emails = add_hpack(vp);
	void *list = add_list(emails); // Fl_Browser or Fl_Check_Browser
	void *reader = add_textbox(emails); // Text box?
*/
	return true;
}

bool update() {
#ifndef NDEBUG
//	cerr << PNAME << ": Update" << endl;
#endif
	return true;
}

bool stop() {
#ifndef NDEBUG
	cerr << PNAME << ": Stop" << endl;
#endif
	return true;
}


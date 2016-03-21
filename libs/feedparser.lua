local LOM = assert(require("lxp.lom"), "LuaExpat doesn't seem to be installed. feedparser kind of needs it to work...")
local XMLElement = (loadfile "./libs/XMLElement.lua")()
local dateparser = (loadfile "./libs/dateparser.lua")()
local URL = (loadfile "./libs/url.lua")()
local tinsert, tremove, tconcat = table.insert, table.remove, table.concat
local pairs, ipairs = pairs, ipairs

--- feedparser, similar to the Universal Feed Parser for python, but a good deal weaker.
-- see http://feedparser.org for details about the Universal Feed Parser
local feedparser= {
	_DESCRIPTION = "RSS and Atom feed parser",
	_VERSION = "feedparser 0.71"
}

local blanky = XMLElement.new() --useful in a whole bunch of places

local function resolve(url, base_url)
	return URL.absolute(base_url, url)
end

local function rebase(el, base_uri)
	local xml_base = el:getAttr('xml:base')
	if not xml_base then return base_uri end
	return resolve(xml_base, base_uri)
end

local function parse_entries(entries_el, format_str, base)
	local entries = {}
	for i, entry_el in ipairs(entries_el) do
		local entry = {enclosures={}, links={}, contributors={}}
		local entry_base = rebase(entry_el, base)
		for i, el in ipairs(entry_el:getChildren('*')) do
			local tag = el:getTag()
			local el_base = rebase(el, entry_base)
			--title
			if tag == 'title' or tag == 'dc:title' or tag =='rdf:title' then --'dc:title' doesn't occur in atom feeds, but whatever.
				entry.title=el:getText()

			--link(s)
			elseif format_str == 'rss' and tag=='link' then
				entry.link=resolve(el:getText(), el_base)
				tinsert(entry.links, {href=entry.link})

			elseif	(format_str=='atom' and tag == 'link') or
					(format_str == 'rss' and tag=='atom:link') then
				local link = {}
				for i, attr in ipairs{'rel','type', 'href','title'} do
					link[attr]= (attr=='href') and resolve(el:getAttr(attr), el_base) or el:getAttr(attr) --uri
				end
				tinsert(entry.links, link)
				if link.rel=='enclosure' then
					tinsert(entry.enclosures, {
						href=link.href,
						length=el:getAttr('length'),
						type=el:getAttr('type')
					})
				end

			--rss enclosures
			elseif format_str == 'rss' and tag=='enclosure' then
				tinsert(entry.enclosures, {
					url=el:getAttr('url'),
					length=el:getAttr('length'),
					type=el:getAttr('type')
				})

			--summary
			elseif	(format_str=='atom' and tag=='summary') or
					(format_str=='rss' and(tag=='description' or tag=='dc:description' or tag=='rdf:description')) then
				entry.summary=el:getText()
				--TODO: summary_detail

			--content
			elseif	(format_str=='atom' and tag=='content') or
					(format_str=='rss' and (tag=='body' or tag=='xhtml:body' or tag == 'fullitem' or tag=='content:encoded')) then
				entry.content=el:getText()
				--TODO: content_detail

			--published
			elseif	(format_str == 'atom' and (tag=='published' or tag=='issued')) or
					(format_str == 'rss' and (tag=='dcterms:issued' or tag=='atom:published' or tag=='atom:issued')) then
				entry.published = el:getText()
				entry.published_parsed=dateparser.parse(entry.published)

			--updated
			elseif	(format_str=='atom' and (tag=='updated' or tag=='modified')) or
					(format_str=='rss' and (tag=='dc:date' or tag=='pubDate' or tag=='dcterms:modified')) then
				entry.updated=el:getText()
				entry.updated_parsed=dateparser.parse(entry.updated)

			elseif tag=='created' or tag=='atom:created' or tag=='dcterms:created' then
				entry.created=el:getText()
				entry.created_parsed=dateparser.parse(entry.created)

			--id
			elseif	(format_str =='atom' and tag=='id') or
					(format_str=='rss' and tag=='guid') then
				entry.id=resolve(el:getText(), el_base)  -- this is a uri, right?...

			--author
			elseif format_str=='rss' and (tag=='author' or tag=='dc:creator') then --author tag should give the author's email. should I respect this?
				entry.author=(el:getChild('name') or el):getText()
				entry.author_detail={
					name=entry.author
				}
			elseif format_str=='atom' and tag=='author' then
				entry.author=(el:getChild('name') or el):getText()
				entry.author_detail = {
					name=entry.author,
					email=(el:getChild('email') or blanky):getText()
				}
				local author_url = (el:getChild('url') or blanky):getText()
				if author_url and author_url ~= "" then entry.author_detail.href=resolve(author_url, rebase(el:getChild('url'), el_base)) end

			elseif tag=='category' or tag=='dc:subject' then
				--todo

			elseif tag=='source' then
				--todo
			end
		end

		--wrap up rss guid
		if format_str == 'rss' and (not entry.id) and entry_el:getAttr('rdf:about') then
			entry.id=resolve(entry_el:getAttr('rdf:about'), entry_base) --uri
		end

		--wrap up entry.link
		for i, link in pairs(entry.links) do
			if link.rel=="alternate" or (not link.rel) or link.rel=="" then
				entry.link=link.href --already resolved.
				break
			end
		end
		if not entry.link and format_str=='rss' then
			entry.link=entry.id
		end
		tinsert(entries, entry)
	end
	return entries
end

local function atom_person_construct(person_el, base_uri)
	local dude ={
		name= (person_el:getChild('name')  or blanky):getText(),
		email=(person_el:getChild('email') or blanky):getText()
	}
	local url_el = person_el:getChild('url')
	if url_el then dude.href=resolve(url_el:getText(), rebase(url_el, base_uri)) end
	return dude
end

local function parse_atom(root, base_uri)
	local res = {}
	local feed = {
		links = {},
		contributors={},
		language = root:getAttr('lang') or root:getAttr('xml:lang')
	}
	local root_base = rebase(root, base_uri)
	res.feed=feed
	res.format='atom'
	local version=(root:getAttr('version') or ''):lower()
	if		version=="1.0" or root:getAttr('xmlns')=='http://www.w3.org/2005/Atom' then res.version='atom10'
	elseif	version=="0.3" then res.version='atom03'
	else						res.version='atom' end

	for i, el in ipairs(root:getChildren('*')) do
		local tag = el:getTag()
		local el_base=rebase(el, root_base)
		if tag == 'title' or tag == 'dc:title' or tag == 'atom10:title' or tag == 'atom03:title' then
			feed.title=el:getText() --sanitize!
			--todo: feed.title_detail

		--link stuff
		elseif tag=='link' then
			local link = {}
			for i, attr in ipairs{'rel','type', 'href','title'} do
				link[attr]= (attr=='href') and resolve(el:getAttr(attr), el_base) or el:getAttr(attr)
			end
			tinsert(feed.links, link)

		--subtitle
		elseif tag == 'subtitle' then
			feed.subtitle=el:getText() --sanitize!
		elseif not feed.subtitle and (tag == 'tagline' or tag =='atom03:tagline' or tag=='dc:description') then
			feed.subtitle=el:getText() --sanitize!

		--rights
		elseif tag == 'copyright' or tag == 'rights' then
			feed.rights=el:getText() --sanitize!

		--generator
		elseif tag == 'generator' then
			feed.generator=el:getText() --sanitize!
		elseif tag == 'admin:generatorAgent' then
			feed.generator = feed.generator or el:getAttr('rdf:resource')

		--info
		elseif tag == 'info' then --whatever, nobody cared, anyway.
			feed.info = el:getText()

		--id
		elseif tag=='id' then
			feed.id=resolve(el:getText(), el_base) --this is a url, right?.,,

		--updated
		elseif tag == 'updated' or tag == 'dc:date' or tag == 'modified' or tag=='rss:pubDate' then
			feed.updated = el:getText()
			feed.updated_parsed=dateparser.parse(feed.updated)

		--author
		elseif tag=='author' or tag=='atom:author' then
			feed.author_detail=atom_person_construct(el, el_base)
			feed.author=feed.author_detail.name

		--contributors
		elseif tag=='contributor' or tag=='atom:contributor' then
			tinsert(feed.contributors, atom_person_construct(el, el_base))

		--icon
		elseif tag=='icon' then
			feed.icon=resolve(el:getText(), el_base)

		--logo
		elseif tag=='logo' then
			feed.logo=resolve(el:getText(), el_base)

		--language
		elseif tag=='language' or tag=='dc:language' then
			feed.language=feed.language or el:getText()

		--licence
		end
	end
	--feed.link (already resolved)
	for i, link in pairs(feed.links) do
		if link.rel=='alternate' or not link.rel or link.rel=='' then
			feed.link=link.href
			break
		end
	end

	res.entries=parse_entries(root:getChildren('entry'),'atom', root_base)
	return res
end

local function parse_rss(root, base_uri)

	local channel = root:getChild({'channel', 'rdf:channel'})
	local channel_base = rebase(channel, base_uri)
	if not channel then return nil, "can't parse that." end

	local feed = {links = {}, contributors={}}
	local res = {
		feed=feed,
		format='rss',
		entries={}
	}

	--this isn't quite right at all.
	if root:getTag():lower()=='rdf:rdf' then
		res.version='rss10'
	else
		res.version='rss20'
	end

	for i, el in ipairs(channel:getChildren('*')) do
		local el_base=rebase(el, channel_base)
		local tag = el:getTag()

		if tag=='link' then
			feed.link=resolve(el:getText(), el_base)
			tinsert(feed.links, {href=feed.link})

		--title
		elseif tag == 'title' or tag == 'dc:title' then
			feed.title=el:getText() --sanitize!

		--subtitle
		elseif tag == 'description' or tag =='dc:description' or tag=='itunes:subtitle' then
			feed.subtitle=el:getText() --sanitize!

		--rights
		elseif tag == 'copyright' or tag == 'dc:rights' then
			feed.rights=el:getText() --sanitize!

		--generator
		elseif tag == 'generator' then
			feed.generator=el:getText()
		elseif tag == 'admin:generatorAgent' then
			feed.generator = feed.generator or el:getAttr('rdf:resource')

		--info (nobody cares...)
		elseif tag == 'feedburner:browserFriendly' then
			feed.info = el:getText()

		--updated
		elseif tag == 'pubDate' or tag == 'dc:date' or tag == 'dcterms:modified' then
			feed.updated = el:getText()
			feed.updated_parsed = dateparser.parse(feed.updated)

		--author
		elseif tag=='managingEditor' or tag =='dc:creator' or tag=='itunes:author' or tag =='dc:creator' or tag=='dc:author' then
			feed.author=tconcat(el:getChildren('text()'))
			feed.author_details={name=feed.author}
		elseif tag=='atom:author' then
			feed.author_details = atom_person_construct(el, el_base)
			feed.author = feed.author_details.name

		--contributors
		elseif tag == 'dc:contributor' then
			tinsert(feed.contributors, {name=el:getText()})
		elseif tag == 'atom:contributor' then
			tinsert(feed.contributors, atom_person_construct(el, el_base))

		--image
		elseif tag=='image' or tag=='rdf:image' then
			feed.image={
				title=el:getChild('title'):getText(),
				link=(el:getChild('link') or blanky):getText(),
				width=(el:getChild('width') or blanky):getText(),
				height=(el:getChild('height') or blanky):getText()
			}
			local url_el = el:getChild('url')
			if url_el then feed.image.href = resolve(url_el:getText(), rebase(url_el, el_base)) end

		--language
		elseif tag=='language' or tag=='dc:language' then
			feed.language=el:getText()

		--licence
		--publisher
		--tags
		end
	end

	res.entries=parse_entries(channel:getChildren('item'),'rss', channel_base)
	return res
end


--- parse feed xml
-- @param xml_string feed xml, as a string
-- @param base_url (optional) source url of the feed. useful when resolving relative links found in feed contents
-- @return table with parsed feed info, or nil, error_message on error.
--		the format of the returned table is much like that on http://feedparser.org, with the major difference that
--		dates are parsed into unixtime. Most other fields are very much the same.
function feedparser.parse(xml_string, base_url)
	local lom, err = LOM.parse(xml_string)
	if not lom then return nil, "couldn't parse xml. lxp says: " .. err or "nothing" end
	local rootElement = XMLElement.new(lom)
	local root_tag = rootElement:getTag():lower()
	if root_tag=='rdf:rdf' or root_tag=='rss' then
		return parse_rss(rootElement, base_url)
	elseif root_tag=='feed' then
		return parse_atom(rootElement, base_url)
	else
		return nil, "unknown feed format"
	end
end

--for the sake of backwards-compatibility, feedparser will export a global reference for lua < 5.3
if _VERSION:sub(-3) < "5.3" then
	_G.feedparser=feedparser
end


return feedparser

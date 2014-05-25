/*
 * libInstPatch
 * Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Moderal Public License
 * as published by the Free Software Foundation; version 2.1
 * of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Moderal Public License for more details.
 *
 * You should have received a copy of the GNU Moderal Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA or on the web at http://www.gnu.org.
 */
/**
 * SECTION: IpatchSF2Mod
 * @short_description: SoundFont modulators
 * @see_also: 
 * @stability: Stable
 *
 * SoundFont modulators are used to define real time MIDI effect controls.
 */
#include <glib.h>
#include <glib-object.h>
#include "IpatchSF2Mod.h"
#include "IpatchSF2Gen.h"
#include "ipatch_priv.h"

/* default modulators */
static IpatchSF2Mod default_mods[] =
{
  { 0x0502, IPATCH_SF2_GEN_ATTENUATION, 960, 0x0, 0 },
  { 0x0102, IPATCH_SF2_GEN_FILTER_CUTOFF, -2400, 0xD02, 0 },
  { 0x000D, IPATCH_SF2_GEN_VIB_LFO_TO_PITCH, 50, 0x0, 0 },
  { 0x0081, IPATCH_SF2_GEN_VIB_LFO_TO_PITCH, 50, 0x0, 0 },
  { 0x0587, IPATCH_SF2_GEN_ATTENUATION, 960, 0x0, 0 },
  { 0x028A, IPATCH_SF2_GEN_PAN, 1000, 0x0, 0 },
  { 0x058B, IPATCH_SF2_GEN_ATTENUATION, 960, 0x0, 0 },
  { 0x00DB, IPATCH_SF2_GEN_REVERB, 200, 0x0, 0 },
  { 0x00DD, IPATCH_SF2_GEN_CHORUS, 200, 0x0, 0 }
  //    { 0x020E, InitialPitch WTF?, 12700, 0x0010, 0 },
};

GType
ipatch_sf2_mod_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_boxed_type_register_static ("IpatchSF2Mod",
				(GBoxedCopyFunc)ipatch_sf2_mod_duplicate,
				(GBoxedFreeFunc)ipatch_sf2_mod_free);

  return (type);
}

GType 
ipatch_sf2_mod_list_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_boxed_type_register_static ("IpatchSF2ModList",
				(GBoxedCopyFunc)ipatch_sf2_mod_list_duplicate,
				(GBoxedFreeFunc)ipatch_sf2_mod_list_boxed_free);

  return (type);
}

/**
 * ipatch_sf2_mod_new:
 *
 * Create a new modulator
 *
 * Returns: New modulator
 */
IpatchSF2Mod *
ipatch_sf2_mod_new (void)
{
  return (g_slice_new0 (IpatchSF2Mod));
}

/**
 * ipatch_sf2_mod_free:
 * @mod: Modulator to free, should not be referenced by any zones.
 *
 * Free an #IpatchSF2Mod structure
 */
void
ipatch_sf2_mod_free (IpatchSF2Mod *mod)
{
  g_return_if_fail (mod != NULL);
  g_slice_free (IpatchSF2Mod, mod);
}

/**
 * ipatch_sf2_mod_duplicate:
 * @mod: Modulator to duplicate
 *
 * Duplicate a modulator
 *
 * Returns: New duplicate modulator
 */
IpatchSF2Mod *
ipatch_sf2_mod_duplicate (const IpatchSF2Mod *mod)
{
  IpatchSF2Mod *newmod;

  g_return_val_if_fail (mod != NULL, NULL);

  newmod = ipatch_sf2_mod_new ();

  newmod->src = mod->src;
  newmod->dest = mod->dest;
  newmod->amount = mod->amount;
  newmod->amtsrc = mod->amtsrc;
  newmod->trans = mod->trans;

  return (newmod);
}

/**
 * ipatch_sf2_mod_list_duplicate:
 * @list: Modulator list to duplicate
 *
 * Duplicates a modulator list (list and modulator data).
 *
 * Returns: (transfer full): New duplicate modulator list which
 *   should be freed with ipatch_sf2_mod_list_free() with @free_mods set to
 *   %TRUE when finished with it.
 */
IpatchSF2ModList *
ipatch_sf2_mod_list_duplicate (const IpatchSF2ModList *list)
{
  IpatchSF2ModList *newlist = NULL;

  while (list)
    {
      newlist = g_slist_prepend (newlist, ipatch_sf2_mod_duplicate
				((IpatchSF2Mod *)(list->data)));
      list = list->next;
    }

  newlist = g_slist_reverse (newlist);

  return (newlist);
}

/**
 * ipatch_sf2_mod_list_override: (skip)
 * @alist: First modulator list
 * @blist: Second modulator list
 * @copy: If %TRUE then modulator data is duplicated
 *
 * Creates a new modulator list by combining @alist and @blist. Modulators
 * in @blist override identical modulators in @alist. If @copy is set then
 * the modulator data is also duplicated (a new IpatchSF2ModList is created).
 *
 * Returns: New IpatchSF2ModList of combined modulator lists.
 * Should be freed with ipatch_sf2_mod_list_free() with the free_mods parameter
 * set to the value of @copy.
 */
IpatchSF2ModList *
ipatch_sf2_mod_list_override (const IpatchSF2ModList *alist, const IpatchSF2ModList *blist,
			      gboolean copy)
{
  IpatchSF2ModList *newlist, *bcopy, *p;
  IpatchSF2Mod *amod, *bmod;

  if (copy) newlist = ipatch_sf2_mod_list_duplicate (blist);
  else newlist = g_slist_copy ((IpatchSF2ModList *)blist);

  if (!newlist)			/* optimize for empty blist */
    {
      if (copy) return (ipatch_sf2_mod_list_duplicate (alist));
      else return (g_slist_copy ((IpatchSF2ModList *)alist));
    }

  bcopy = newlist;
  while (alist)			/* loop over alist */
    {
      amod = (IpatchSF2Mod *)(alist->data);
      p = bcopy;
      while (p)
	{
	  bmod = (IpatchSF2Mod *)(p->data);

	  if (IPATCH_SF2_MOD_ARE_IDENTICAL (amod, bmod)) break;
	  p = p->next;
	}

      if (!p)			/* no duplicate found? */
	newlist = g_slist_prepend (newlist, copy ? ipatch_sf2_mod_duplicate
				   (amod) : amod);
      alist = alist->next;
    }

  return (newlist);
}

/**
 * ipatch_sf2_mod_list_override_copy: (rename-to ipatch_sf2_mod_list_override)
 * @alist: First modulator list
 * @blist: Second modulator list
 *
 * Creates a new modulator list by combining @alist and @blist. Modulators
 * in @blist override identical modulators in @alist. If @copy is set then
 * the modulator data is also duplicated (a new IpatchSF2ModList is created).
 *
 * Returns: (transfer full): New IpatchSF2ModList of combined modulator lists.
 * Should be freed with ipatch_sf2_mod_list_free() with the free_mods parameter
 * set to %TRUE.
 *
 * Since: 1.1.0
 */
IpatchSF2ModList *
ipatch_sf2_mod_list_override_copy (const IpatchSF2ModList *alist, const IpatchSF2ModList *blist)
{
  return (ipatch_sf2_mod_list_override (alist, blist, TRUE));
}

/**
 * ipatch_sf2_mod_list_offset:
 * @alist: First modulator list
 * @blist: Second modulator list
 *
 * Creates a new modulator list by combining @list and @blist. Modulators
 * in @blist offset (amounts are added) identical modulators in @alist.
 * Operation is non-destructive as a new list is created and modulator data
 * is duplicated.
 *
 * NOTE: Optimized for empty @blist.
 *
 * Returns: (transfer full): New IpatchSF2ModList of combined modulator lists.
 *   Should be freed with ipatch_sf2_mod_list_free() with @free_mods set to
 *   %TRUE when finished with it.
 */
IpatchSF2ModList *
ipatch_sf2_mod_list_offset (const IpatchSF2ModList *alist, const IpatchSF2ModList *blist)
{
  IpatchSF2ModList *newlist, *acopy, *p;
  IpatchSF2Mod *amod, *bmod;
  int add;

  newlist = ipatch_sf2_mod_list_duplicate (alist);
  if (!blist) return (newlist);	/* optimize for empty blist */

  acopy = newlist;
  while (blist)			/* loop over alist */
    {
      bmod = (IpatchSF2Mod *)(blist->data);
      p = acopy;
      while (p)
	{
	  amod = (IpatchSF2Mod *)(p->data);

	  if (IPATCH_SF2_MOD_ARE_IDENTICAL (amod, bmod))
	    {
	      /* offset (add) the modulator amount */
	      add = amod->amount + bmod->amount;
	      add = CLAMP (add, -32768, 32767);
	      amod->amount = add;
	      break;
	    }
	  p = p->next;
	}

      /* no duplicate found? */
      if (!p) newlist = g_slist_prepend (newlist,
					 ipatch_sf2_mod_duplicate (bmod));

      blist = blist->next;
    }

  return (newlist);
}

/**
 * ipatch_sf2_mod_list_free: (skip)
 * @list: Modulator list to free
 * @free_mods: If %TRUE then the modulators themselves are freed, %FALSE
 *   makes this function act just like g_slist_free() (only the list is
 *   freed not the modulators).
 *
 * Free a list of modulators
 */
void
ipatch_sf2_mod_list_free (IpatchSF2ModList *list, gboolean free_mods)
{
  IpatchSF2ModList *p;

  if (free_mods)
    {
      p = list;
      while (p)
	{
	  ipatch_sf2_mod_free ((IpatchSF2Mod *)(p->data));
	  p = g_slist_delete_link (p, p);
	}
    }
  else g_slist_free (list);
}

/**
 * ipatch_sf2_mod_list_boxed_free:
 * @list: Modulator list to free
 *
 * Like ipatch_sf2_mod_list_free() but used for boxed type declaration and so
 * therefore frees all modulators in the list.
 */
void
ipatch_sf2_mod_list_boxed_free (IpatchSF2ModList *list)
{
  ipatch_sf2_mod_list_free (list, TRUE);
}

/**
 * ipatch_sf2_mod_list_insert:
 * @mods: (transfer full): Modulator list to insert into
 * @modvals: Modulator values to insert (a new modulator is created and the values
 *   are copied to it)
 * @pos: Index position in zone's modulator list to insert
 *   (0 = first, < 0 = last)
 *
 * Inserts a modulator into a modulator list. Does not check for
 * duplicates! The modulator is not used directly, a new one is created and
 * the values in @mod are copied to it.
 *
 * Returns: (transfer full): New start (root) of @mods list.
 */
IpatchSF2ModList *
ipatch_sf2_mod_list_insert (IpatchSF2ModList *mods, const IpatchSF2Mod *modvals, int pos)
{
  IpatchSF2Mod *newmod;

  g_return_val_if_fail (modvals != NULL, mods);

  newmod = ipatch_sf2_mod_duplicate (modvals);
  return (g_slist_insert (mods, newmod, pos));
}

/**
 * ipatch_sf2_mod_list_remove:
 * @mods: (transfer full): Modulator list to remove from
 * @modvals: Values of modulator to remove
 * @changed: (out) (allow-none): Pointer to store bool of whether the list was changed
 *   (%NULL to ignore)
 *
 * Remove a modulator from a modulator list. The modulator values in @modvals
 * are used to search the modulator list. The first modulator
 * that matches all fields in @modvals is removed.
 *
 * Returns: (transfer full): New start (root) of @mods list.
 */
IpatchSF2ModList *
ipatch_sf2_mod_list_remove (IpatchSF2ModList *mods, const IpatchSF2Mod *modvals,
			    gboolean *changed)
{
  IpatchSF2Mod *mod;
  IpatchSF2ModList *p;

  if (changed) *changed = FALSE;

  g_return_val_if_fail (modvals != NULL, mods);

  for (p = mods; p; p = g_slist_next (p))
    {
      mod = (IpatchSF2Mod *)(p->data);

      if (IPATCH_SF2_MOD_ARE_IDENTICAL_AMOUNT (mod, modvals))
	{
	  ipatch_sf2_mod_free (mod);
	  if (changed) *changed = TRUE;
	  return (g_slist_delete_link (mods, p));
	}
    }

  return (mods);
}

/**
 * ipatch_sf2_mod_list_change:
 * @mods: Modulator list to change a modulator in
 * @oldvals: Current values of modulator to set
 * @newvals: New modulator values
 *
 * Sets the values of an existing modulator in a modulator list. The list
 * is searched for a modulator that matches the values in @oldvals. If a
 * modulator is found its values are set to those in @newvals. If it is not
 * found, nothing is done.
 *
 * Returns: %TRUE if changed, %FALSE otherwise (no match)
 */
gboolean
ipatch_sf2_mod_list_change (IpatchSF2ModList *mods, const IpatchSF2Mod *oldvals,
			    const IpatchSF2Mod *newvals)
{
  IpatchSF2Mod *mod;
  IpatchSF2ModList *p;

  g_return_val_if_fail (oldvals != NULL, FALSE);
  g_return_val_if_fail (newvals != NULL, FALSE);

  for (p = mods; p; p = p->next)
    {
      mod = (IpatchSF2Mod *)(p->data);

      if (IPATCH_SF2_MOD_ARE_IDENTICAL_AMOUNT (mod, oldvals))
	{
	  *mod = *newvals;	/* replace values in modulator */
	  return (TRUE);
	}
    }

  return (FALSE);
}

/**
 * ipatch_sf2_mod_list_get_default:
 *
 * Get the list of default instrument modulators.
 *
 * Returns: (transfer none): The list of default
 *   modulators. The same modulator list is returned on subsequent calls and
 *   should not be modified or freed.
 */
G_CONST_RETURN IpatchSF2ModList *
ipatch_sf2_mod_list_get_default (void)
{
  static IpatchSF2ModList *list = NULL;
  int i;

  if (!list)
    for (i = sizeof (default_mods) / sizeof (IpatchSF2Mod) - 1; i >= 0 ; i--)
      list = g_slist_prepend (list, &default_mods[i]);

  return (list);
}

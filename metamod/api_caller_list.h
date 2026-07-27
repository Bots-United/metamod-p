/*
 * Copyright (c) 2026 Jussi Kivilinna
 *
 *    This file is part of "Metamod All-Mod-Support"-patch for Metamod.
 *
 *    Metamod is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    Metamod is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Metamod; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

//
// One entry per (return type, argument pack) combination. No include guard
// by design: the includer defines BEGIN_API_CALLER_FUNC and
// END_API_CALLER_FUNC* first, and so decides what an entry expands to.
// api_hook.cpp expands them into the real callers, test_api_hook.cpp a
// second time into reference callers passing arguments one by one, so a
// pack struct drifting from its call signature shows up as a differing
// argument block instead of silent corruption.
//

//-
BEGIN_API_CALLER_FUNC(void, ipV)
END_API_CALLER_FUNC_void( (int, const void*, ...), (p->i1, p->p1, p->str) )

//-
BEGIN_API_CALLER_FUNC(void, 2pV)
END_API_CALLER_FUNC_void( (const void*, const void*, ...), (p->p1, p->p2, p->str) )

//-
BEGIN_API_CALLER_FUNC(void, void)
END_API_CALLER_FUNC_noargs_void()

BEGIN_API_CALLER_FUNC(ptr, void)
END_API_CALLER_FUNC_noargs(void*)

BEGIN_API_CALLER_FUNC(int, void)
END_API_CALLER_FUNC_noargs(int)

BEGIN_API_CALLER_FUNC(float, void)
END_API_CALLER_FUNC_noargs(float)

//-
BEGIN_API_CALLER_FUNC(float, 2f)
END_API_CALLER_FUNC( float, (float, float), (p->f1, p->f2) )

//-
BEGIN_API_CALLER_FUNC(void, 2i)
END_API_CALLER_FUNC_void( (int, int), (p->i1, p->i2) );

BEGIN_API_CALLER_FUNC(int, 2i)
END_API_CALLER_FUNC(int, (int, int), (p->i1, p->i2) );

//-
BEGIN_API_CALLER_FUNC(void, 2i2p)
END_API_CALLER_FUNC_void( (int, int, const void*, const void*), (p->i1, p->i2, p->p1, p->p2) );

//-
BEGIN_API_CALLER_FUNC(void, 2i2pi2p)
END_API_CALLER_FUNC_void( (int, int, const void*, const void*, int, const void*, const void*), (p->i1, p->i2, p->p1, p->p2, p->i3, p->p3, p->p4) );

//-
BEGIN_API_CALLER_FUNC(void, 2p)
END_API_CALLER_FUNC_void( (const void*, const void*), (p->p1, p->p2) );

BEGIN_API_CALLER_FUNC(ptr, 2p)
END_API_CALLER_FUNC(void*, (const void*, const void*), (p->p1, p->p2) );

BEGIN_API_CALLER_FUNC(int, 2p)
END_API_CALLER_FUNC(int, (const void*, const void*), (p->p1, p->p2) );

//-
BEGIN_API_CALLER_FUNC(void, 2p2f)
END_API_CALLER_FUNC_void( (const void*, const void*, float, float), (p->p1, p->p2, p->f1, p->f2) );

//-
BEGIN_API_CALLER_FUNC(void, 2p2i2p)
END_API_CALLER_FUNC_void( (const void*, const void*, int, int, const void*, const void*), (p->p1, p->p2, p->i1, p->i2, p->p3, p->p4) );

//-
BEGIN_API_CALLER_FUNC(void, 2p3fus2uc)
END_API_CALLER_FUNC_void( (const void*, const void*, float, float, float, unsigned short, unsigned char, unsigned char), (p->p1, p->p2, p->f1, p->f2, p->f3, p->us1, p->uc1, p->uc2) );

//-
BEGIN_API_CALLER_FUNC(ptr, 2pf)
END_API_CALLER_FUNC(void*, (const void*, const void*, float), (p->p1, p->p2, p->f1) );

//-
BEGIN_API_CALLER_FUNC(void, 2pfi)
END_API_CALLER_FUNC_void( (const void*, const void*, float, int), (p->p1, p->p2, p->f1, p->i1) );

//-
BEGIN_API_CALLER_FUNC(void, 2pi)
END_API_CALLER_FUNC_void( (const void*, const void*, int), (p->p1, p->p2, p->i1) );

BEGIN_API_CALLER_FUNC(int, 2pi)
END_API_CALLER_FUNC(int, (const void*, const void*, int), (p->p1, p->p2, p->i1) );

//-
BEGIN_API_CALLER_FUNC(void, 2pui)
END_API_CALLER_FUNC_void( (const void*, const void*, unsigned int), (p->p1, p->p2, p->ui1) );

//-
BEGIN_API_CALLER_FUNC(void, 2pi2p)
END_API_CALLER_FUNC_void( (const void*, const void*, int, const void*, const void*), (p->p1, p->p2, p->i1, p->p3, p->p4) );

//-
BEGIN_API_CALLER_FUNC(void, 2pif2p)
END_API_CALLER_FUNC_void( (const void*, const void*, int, float, const void*, const void*), (p->p1, p->p2, p->i1, p->f1, p->p3, p->p4) );

//-
BEGIN_API_CALLER_FUNC(int, 3i)
END_API_CALLER_FUNC(int, (int, int, int), (p->i1, p->i2, p->i3) );

//-
BEGIN_API_CALLER_FUNC(void, 3p)
END_API_CALLER_FUNC_void( (const void*, const void*, const void*), (p->p1, p->p2, p->p3) );

BEGIN_API_CALLER_FUNC(ptr, 3p)
END_API_CALLER_FUNC(void*, (const void*, const void*, const void*), (p->p1, p->p2, p->p3) );

BEGIN_API_CALLER_FUNC(int, 3p)
END_API_CALLER_FUNC(int, (const void*, const void*, const void*), (p->p1, p->p2, p->p3) );

//-
BEGIN_API_CALLER_FUNC(void, 3p2f2i)
END_API_CALLER_FUNC_void( (const void*, const void*, const void*, float, float, int, int), (p->p1, p->p2, p->p3, p->f1, p->f2, p->i1, p->i2) );

//-
BEGIN_API_CALLER_FUNC(int, 3pi2p)
END_API_CALLER_FUNC(int, (const void*, const void*, const void*, int, const void*, const void*), (p->p1, p->p2, p->p3, p->i1, p->p4, p->p5) );

//-
BEGIN_API_CALLER_FUNC(void, 4p)
END_API_CALLER_FUNC_void( (const void*, const void*, const void*, const void*), (p->p1, p->p2, p->p3, p->p4) );

BEGIN_API_CALLER_FUNC(int, 4p)
END_API_CALLER_FUNC(int, (const void*, const void*, const void*, const void*), (p->p1, p->p2, p->p3, p->p4) );

//-
BEGIN_API_CALLER_FUNC(void, 4pi)
END_API_CALLER_FUNC_void( (const void*, const void*, const void*, const void*, int), (p->p1, p->p2, p->p3, p->p4, p->i1) );

BEGIN_API_CALLER_FUNC(int, 4pi)
END_API_CALLER_FUNC(int, (const void*, const void*, const void*, const void*, int), (p->p1, p->p2, p->p3, p->p4, p->i1) );

//-
BEGIN_API_CALLER_FUNC(void, f)
END_API_CALLER_FUNC_void( (float), (p->f1) );

//-
BEGIN_API_CALLER_FUNC(void, i)
END_API_CALLER_FUNC_void( (int), (p->i1) );

BEGIN_API_CALLER_FUNC(int, i)
END_API_CALLER_FUNC(int, (int), (p->i1) );

BEGIN_API_CALLER_FUNC(ptr, i)
END_API_CALLER_FUNC(void*, (int), (p->i1) );

BEGIN_API_CALLER_FUNC(uint, ui)
END_API_CALLER_FUNC(unsigned int, (unsigned int), (p->ui1) );

BEGIN_API_CALLER_FUNC(ptr, ui)
END_API_CALLER_FUNC(void*, (unsigned int), (p->ui1) );

//-
BEGIN_API_CALLER_FUNC(ulong, ul)
END_API_CALLER_FUNC(unsigned long, (unsigned long), (p->ul1) );

//-
BEGIN_API_CALLER_FUNC(void, i2p)
END_API_CALLER_FUNC_void( (int, const void*, const void*), (p->i1, p->p1, p->p2) );

BEGIN_API_CALLER_FUNC(int, i2p)
END_API_CALLER_FUNC(int, (int, const void*, const void*), (p->i1, p->p1, p->p2) );

//-
BEGIN_API_CALLER_FUNC(void, i3p)
END_API_CALLER_FUNC_void( (int, const void*, const void*, const void*), (p->i1, p->p1, p->p2, p->p3) );

//-
BEGIN_API_CALLER_FUNC(void, ip)
END_API_CALLER_FUNC_void( (int, const void*), (p->i1, p->p1) );

BEGIN_API_CALLER_FUNC(ushort, ip)
END_API_CALLER_FUNC( unsigned short, (int, const void*), (p->i1, p->p1) );

BEGIN_API_CALLER_FUNC(int, ip)
END_API_CALLER_FUNC( int, (int, const void*), (p->i1, p->p1) );

//-
BEGIN_API_CALLER_FUNC(void, ipusf2p2f4i)
END_API_CALLER_FUNC_void( (int, const void*, unsigned short, float, const void*, const void*, float, float, int, int, int, int), (p->i1, p->p1, p->us1, p->f1, p->p2, p->p3, p->f2, p->f3, p->i2, p->i3, p->i4, p->i5) );

//-
BEGIN_API_CALLER_FUNC(void, p)
END_API_CALLER_FUNC_void( (const void*), (p->p1) );

BEGIN_API_CALLER_FUNC(ptr, p)
END_API_CALLER_FUNC(void*, (const void*), (p->p1) );

BEGIN_API_CALLER_FUNC(char, p)
END_API_CALLER_FUNC(char, (const void*), (p->p1) );

BEGIN_API_CALLER_FUNC(int, p)
END_API_CALLER_FUNC(int, (const void*), (p->p1) );

BEGIN_API_CALLER_FUNC(uint, p)
END_API_CALLER_FUNC(unsigned int, (const void*), (p->p1) );

BEGIN_API_CALLER_FUNC(float, p)
END_API_CALLER_FUNC(float, (const void*), (p->p1) );

//-
BEGIN_API_CALLER_FUNC(void, p2f)
END_API_CALLER_FUNC_void( (const void*, float, float), (p->p1, p->f1, p->f2) );

//-
BEGIN_API_CALLER_FUNC(int, p2fi)
END_API_CALLER_FUNC(int, (const void*, float, float, int), (p->p1, p->f1, p->f2, p->i1) );

//-
BEGIN_API_CALLER_FUNC(void, p2i)
END_API_CALLER_FUNC_void( (const void*, int, int), (p->p1, p->i1, p->i2) );

//-
BEGIN_API_CALLER_FUNC(void, p3i)
END_API_CALLER_FUNC_void( (const void*, int, int, int), (p->p1, p->i1, p->i2, p->i3) );

//-
BEGIN_API_CALLER_FUNC(void, p4i)
END_API_CALLER_FUNC_void( (const void*, int, int, int, int), (p->p1, p->i1, p->i2, p->i3, p->i4) );

//-
BEGIN_API_CALLER_FUNC(void, puc)
END_API_CALLER_FUNC_void( (const void*, unsigned char), (p->p1, p->uc1) );

//-
BEGIN_API_CALLER_FUNC(void, pf)
END_API_CALLER_FUNC_void( (const void*, float), (p->p1, p->f1) );

//-
BEGIN_API_CALLER_FUNC(void, pfp)
END_API_CALLER_FUNC_void( (const void*, float, const void*), (p->p1, p->f1, p->p2) );

//-
BEGIN_API_CALLER_FUNC(void, pi)
END_API_CALLER_FUNC_void( (const void*, int), (p->p1, p->i1) );

BEGIN_API_CALLER_FUNC(ptr, pi)
END_API_CALLER_FUNC(void*, (const void*, int), (p->p1, p->i1) );

BEGIN_API_CALLER_FUNC(int, pi)
END_API_CALLER_FUNC(int, (const void*, int), (p->p1, p->i1) );

//-
BEGIN_API_CALLER_FUNC(void, pi2p)
END_API_CALLER_FUNC_void( (const void*, int, const void*, const void*), (p->p1, p->i1, p->p2, p->p3) );

//-
BEGIN_API_CALLER_FUNC(int, pi2p2ip)
END_API_CALLER_FUNC(int, (const void*, int, const void*, const void*, int, int, const void*), (p->p1, p->i1, p->p2, p->p3, p->i2, p->i3, p->p4) );

//-
BEGIN_API_CALLER_FUNC(void, pip)
END_API_CALLER_FUNC_void( (const void*, int, const void*), (p->p1, p->i1, p->p2) );

BEGIN_API_CALLER_FUNC(ptr, pip)
END_API_CALLER_FUNC(void*, (const void*, int, const void*), (p->p1, p->i1, p->p2) );

//-
BEGIN_API_CALLER_FUNC(void, pip2f2i)
END_API_CALLER_FUNC_void( (const void*, int, const void*, float, float, int, int), (p->p1, p->i1, p->p2, p->f1, p->f2, p->i2, p->i3) );

//-
BEGIN_API_CALLER_FUNC(void, pip2f4i2p)
END_API_CALLER_FUNC_void( (const void*, int, const void*, float, float, int, int, int, int, const void*, const void*), (p->p1, p->i1, p->p2, p->f1, p->f2, p->i2, p->i3, p->i4, p->i5, p->p3, p->p4) );

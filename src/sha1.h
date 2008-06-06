/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is SHA 180-1 Header File
 * 
 * The Initial Developer of the Original Code is Paul Kocher of
 * Cryptography Research.  Portions created by Paul Kocher are 
 * Copyright (C) 1995-9 by Cryptography Research, Inc.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 *
 *     Paul Kocher
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

/*
Copied from the git sources, with the following revision history:
commit 77ab8798d3f8df39877235be17bb6e70077aaba2
Author: Junio C Hamano <junkio@cox.net>
Date:   Tue Nov 1 10:56:03 2005 -0800

    Fix constness of input in mozilla-sha1/sha1.c::SHA1_Update().

    Among the three of our own implementations, only this one lacked
    "const" from the second argument.

    Signed-off-by: Junio C Hamano <junkio@cox.net>

commit cef661fc799a3a13ffdea4a3f69f1acd295de53d
Author: Linus Torvalds <torvalds@ppc970.osdl.org>
Date:   Thu Apr 21 12:33:22 2005 -0700

    Add support for alternate SHA1 library implementations.

    This one includes the Mozilla SHA1 implementation sent in by Edgar Toernig.
    It's dual-licenced under MPL-1.1 or GPL, so in the context of git, we
    obviously use the GPL version.

    Side note: the Mozilla SHA1 implementation is about twice as fast as the
    default openssl one on my G5, but the default openssl one has optimized
    x86 assembly language on x86. So choose wisely.

*/

typedef struct {
  unsigned int H[5];
  unsigned int W[80];
  int lenW;
  unsigned int sizeHi,sizeLo;
} SHA_CTX;

void SHA1_Init(SHA_CTX *ctx);
void SHA1_Update(SHA_CTX *ctx, const void *dataIn, int len);
void SHA1_Final(unsigned char hashout[20], SHA_CTX *ctx);

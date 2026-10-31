// SPDX-License-Identifier: GPL-3.0-only
// (A) ROBUSTNESS — the important fix.
// Kernel.sc:34-49:
//     if(aClass ... findMethod(\initClass).notNil, { aClass.initClass; });
//     if(aClass.subclasses.notNil, { aClass.subclasses.do({ |c| this.initClassTree(c) }) });
// There is NO try: if ONE initClass throws (e.g. adclib/Mouse:initClass ->
// Window.screenBounds -> missing Qt primitive _QWindow_ScreenBounds), the
// exception bubbles up through Process:startup and ABORTS the rest of the walk:
// every sibling/later class ends up WITHOUT initClass (symptom: "Class tree
// inited in ..." is never printed). A single GUI quark can disable initClass for
// an arbitrary fraction of the user's library.
// This override makes one failure harmless to the rest.
+ Class {
	*initClassTree { |aClass|
		var implementsInitClass;
		if(classesInited.isNil, { classesInited = IdentitySet.new });
		if(classesInited.includes(aClass).not, {
			classesInited.add(aClass);
			if(aClass.isMetaClass.not and: { aClass.class.findMethod(\initClass).notNil }, {
				try {
					aClass.initClass;
				} { |err|
					"SCMaxCompat: initClass failed in % (ignored, carrying on)\n".postf(aClass);
				};
			});
			if(aClass.subclasses.notNil, {
				aClass.subclasses.do({ |class| this.initClassTree(class) });
			});
		});
	}
}

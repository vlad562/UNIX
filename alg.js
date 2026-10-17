class Solution {
	/**
	 * @param {ListNode} head
	 * @param {number} val
	 * @return {ListNode}
	 */
	removeElements(head, val) {
		if (head === null) return null
		head.next = this.removeElements(head.next, val)
		return head.val !== val ? head : head.next
	}
}

class ListNode {
	constructor(val = 0, next = null) {
		this.val = val
		this.next = next
	}
}

const s = new Solution()
let head = new ListNode(
	2,
	new ListNode(
		1,
		new ListNode(4, new ListNode(1, new ListNode(2, new ListNode(3)))),
	),
)

head = new ListNode(1, new ListNode(1))

head =  new ListNode(1, new ListNode(2, new ListNode(6, new ListNode(3, new ListNode(4, new ListNode(5, new ListNode(6)))))));

console.log(s.removeElements(head, 6))

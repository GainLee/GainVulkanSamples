/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Gain
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
package com.gain.vulkan

import android.os.Bundle
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import com.gain.vulkan.placeholder.PlaceholderContent

/**
 * A fragment representing a list of Items.
 */
class ChooseFragment : Fragment() {

    private var actionType: PlaceholderContent.ActionType? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        arguments?.let {
            var type = it.getInt(ARG_TYPE)
            actionType = PlaceholderContent.ActionType.fromInt(type)
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        val view = inflater.inflate(R.layout.fragment_choose_list, container, false)

        // Set the adapter
        if (view is RecyclerView) {
            with(view) {
                layoutManager = LinearLayoutManager(context)
                adapter = if(actionType==null) {
                    MyItemRecyclerViewAdapter(PlaceholderContent.ROOT_ITEMS) {
                        onItemClick(it)
                    }
                } else {
                    MyItemRecyclerViewAdapter(actionType!!.items) { fragment ->
                        onItemClick(fragment)
                    }
                }
            }
        }
        return view
    }

    private fun onItemClick(fragment:Fragment) {
        activity!!.supportFragmentManager.beginTransaction()
            .replace(
                (view!!.parent as ViewGroup).id,
                fragment
            )
            .addToBackStack(null)
            .commit()
    }

    companion object {

        const val ARG_TYPE = "type"

        @JvmStatic
        fun newInstance(type: Int) =
            ChooseFragment().apply {
                arguments = Bundle().apply {
                    putInt(ARG_TYPE, type)
                }
            }
    }
}